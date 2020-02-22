#include "components/ecs.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "common/find.h"

namespace ECS
{
    static Array<u16> ms_versions;
    static Queue<i32> ms_free;

    static i32 ms_typeCount;
    static Guid ms_guids[kMaxTypes];
    static i32 ms_strides[kMaxTypes];
    static Array<u16> ms_rowVersions[kMaxTypes];
    static Array<u8> ms_rowBytes[kMaxTypes];

    static bool IsSingleThreaded()
    {
        return (ThreadId() == 0) && (NumActiveThreads() == 0);
    }

    struct System final : ISystem
    {
        System() : ISystem("ECS", { "TaskSystem" }) {}
        void Init() final
        {
            ms_versions.Init(Alloc_Tlsf, 1024);
            ms_free.Init(Alloc_Tlsf, 1024);
        }
        void Update() final
        {
        }
        void Shutdown() final
        {
            for (i32 i = 0; i < ms_typeCount; ++i)
            {
                ms_rowBytes[i].Reset();
                ms_rowVersions[i].Reset();
            }

            ms_versions.Reset();
            ms_free.Reset();
            ms_typeCount = 0;
        }
    };

    static System ms_system;

    static i32 Match(i32 entity, i32 component)
    {
        i32 cmp = entity - component;
        cmp |= cmp >> 16;
        cmp |= cmp >> 8;
        cmp |= cmp >> 4;
        cmp |= cmp >> 2;
        cmp |= cmp >> 1;
        cmp = ~cmp;
        return 1 & entity & cmp;
    }

    ComponentId RegisterType(Guid guid, i32 sizeOf)
    {
        ASSERT(IsSingleThreaded());
        ASSERT(!IsNull(guid));
        ASSERT(sizeOf > 0);

        i32 i = RFind(ARGS(ms_guids), guid);
        if (i == -1)
        {
            ASSERT(ms_typeCount < kMaxTypes);
            i = ms_typeCount++;
            ms_guids[i] = guid;
            ms_strides[i] = sizeOf;
            ms_rowBytes[i].Init(Alloc_Tlsf);
            ms_rowVersions[i].Init(Alloc_Tlsf);
        }

        return { i };
    }

    i32 EntityCount() { return ms_versions.size(); }

    bool IsCurrent(Entity entity)
    {
        ASSERT(entity.version & 1);
        return ms_versions[entity.index] == entity.version;
    }

    Entity Create()
    {
        ASSERT(IsSingleThreaded());

        Entity entity = { 0, 0 };
        if (ms_free.TryPop(entity.index))
        {
            entity.version = ++ms_versions[entity.index];
            ASSERT(entity.version & 1);
            return entity;
        }

        entity.version = 1;
        entity.index = ms_versions.PushBack(entity.version);

        const i32 typeCount = ms_typeCount;
        for (i32 i = 0; i < typeCount; ++i)
        {
            Array<u16>& versions = ms_rowVersions[i];
            Array<u8>& bytes = ms_rowBytes[i];
            const i32 stride = ms_strides[i];
            while (entity.index >= versions.size())
            {
                versions.PushBack(0);
                bytes.ResizeRel(stride);
            }
        }

        return entity;
    }

    bool Destroy(Entity entity)
    {
        ASSERT(IsSingleThreaded());
        u16& rVersion = ms_versions[entity.index];
        if (rVersion == entity.version)
        {
            ++rVersion;
            ms_free.Push(entity.index);
            return true;
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        const u16 version = ms_rowVersions[id.Value][entity.index];
        return entity.version == version;
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(IsSingleThreaded());
        ASSERT(entity.version & 1);

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        u16& rVersion = ms_rowVersions[iRow][entity.index];

        ASSERT((rVersion & 1) == 0);
        if ((entity.version - rVersion) > 0)
        {
            rVersion = entity.version;
            memset(ptr, 0, sizeOf);
            return true;
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        ASSERT(IsSingleThreaded());
        ASSERT(entity.version & 1);
        u16& rVersion = ms_rowVersions[id.Value][entity.index];
        if (rVersion == entity.version)
        {
            ++rVersion;
            return true;
        }
        return false;
    }

    void* Get(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        const u16 version = ms_rowVersions[iRow][entity.index];
        return (version == entity.version) ? ptr : nullptr;
    }

    ForEachTask::ForEachTask(
        std::initializer_list<ComponentId> all,
        std::initializer_list<ComponentId> none) : ITask(0, 0)
    {
        m_all.Init(Alloc_Tlsf);
        m_none.Init(Alloc_Tlsf);
        Copy(m_all, all);
        Copy(m_none, none);
        Setup();
    }

    ForEachTask::~ForEachTask()
    {
        m_all.Reset();
        m_none.Reset();
    }

    void ForEachTask::Setup()
    {
        SetRange(0, ms_versions.size());
    }

    static u16 AnyBit(u16 x)
    {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        return x & 1;
    }

    static u16 NoBit(u16 x)
    {
        return (~AnyBit(x)) & 1;
    }

    void ForEachTask::Execute(i32 begin, i32 end)
    {
        const Slice<const ComponentId> all = m_all;
        const Slice<const ComponentId> none = m_none;
        const u16* const versions = ms_versions.begin();
        const Array<u16>* const rowVersions = ms_rowVersions;

        for (i32 i = begin; i < end; ++i)
        {
            const u16 version = versions[i];
            u16 match = version & 1;
            for (ComponentId id : all)
            {
                match *= NoBit(version - rowVersions[id.Value][i]);
            }
            for (ComponentId id : none)
            {
                match *= AnyBit(version - rowVersions[id.Value][i]);
            }
            if (match)
            {
                OnEntity({ i, version });
            }
        }
    }
};
