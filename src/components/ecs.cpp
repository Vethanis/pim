#include "components/ecs.h"
#include "containers/array.h"
#include "containers/mtqueue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "common/find.h"

namespace ECS
{
    static constexpr i32 kPacketSize = 128;
    static constexpr i32 kMaxTypes = 256;

    static Array<i32> ms_versions;
    static MtQueue<i32> ms_free;

    static i32 ms_typeCount;
    static Guid ms_guids[kMaxTypes];
    static i32 ms_strides[kMaxTypes];
    static Array<i32> ms_rowVersions[kMaxTypes];
    static Array<u8> ms_rowBytes[kMaxTypes];

    struct System final : ISystem
    {
        System() : ISystem("ECS") {}
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

    static bool IsNewer(i32 prev, i32 next) { return (next - prev) > 0; }

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
        Entity entity = { 0, 0 };
        if (ms_free.TryPop(entity.index))
        {
            entity.version = 1 + Inc(ms_versions[entity.index], MO_Relaxed);
            ASSERT(entity.version & 1);
            return entity;
        }

        entity.version = 1;
        entity.index = ms_versions.PushBack(entity.version);

        const i32 typeCount = ms_typeCount;
        for(i32 i = 0; i < typeCount; ++i)
        {
            Array<i32>& versions = ms_rowVersions[i];
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
        if (CmpExStrong(ms_versions[entity.index], entity.version, entity.version + 1))
        {
            ms_free.Push(entity.index);
            return true;
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        return entity.version == Load(ms_rowVersions[id.Value][entity.index], MO_Relaxed);
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        i32& rVersion = ms_rowVersions[iRow][entity.index];

        i32 version = Load(rVersion, MO_Relaxed);
        ASSERT(!(version & 1));
        if ((entity.version - version) > 0)
        {
            if (CmpExStrong(rVersion, version, entity.version, MO_Acquire))
            {
                memset(ptr, 0, sizeOf);
                return true;
            }
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        i32& rVersion = ms_rowVersions[id.Value][entity.index];
        return CmpExStrong(rVersion, entity.version, entity.version + 1, MO_Release);
    }

    void* Get(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        i32& rVersion = ms_rowVersions[iRow][entity.index];

        if (Load(rVersion, MO_Relaxed) == entity.version)
        {
            return ptr;
        }
        return nullptr;
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

    static i32 AnyBit(i32 x)
    {
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x & 1;
    }

    static i32 NoBit(i32 x)
    {
        return (~AnyBit(x)) & 1;
    }

    void ForEachTask::Execute(i32 begin, i32 end)
    {
        const Slice<const ComponentId> all = m_all;
        const Slice<const ComponentId> none = m_none;
        const Array<i32>* const rowVersions = ms_rowVersions;
        const i32* const versions = ms_versions.begin();

        i32 packet[kPacketSize];

        i32 a = begin;
        i32 b = Min(a + kPacketSize, end);
        while (a < b)
        {
            i32 j = 0;
            for (i32 i = a; i < b; ++i)
            {
                packet[j++] = Load(versions[i], MO_Relaxed) & 1;
            }

            for (ComponentId id : all)
            {
                j = 0;
                const i32* const row = rowVersions[id.Value].begin();
                for (i32 i = a; i < b; ++i)
                {
                    packet[j++] &= NoBit(Load(versions[i], MO_Relaxed) - Load(row[i], MO_Relaxed));
                }
            }

            for (ComponentId id : none)
            {
                j = 0;
                const i32* const row = rowVersions[id.Value].begin();
                for (i32 i = a; i < b; ++i)
                {
                    packet[j++] &= AnyBit(Load(versions[i], MO_Relaxed) - Load(row[i], MO_Relaxed));
                }
            }

            j = 0;
            for (i32 i = a; i < b; ++i)
            {
                if (packet[j++])
                {
                    Entity entity;
                    entity.index = i;
                    entity.version = Load(versions[i], MO_Relaxed);
                    OnEntity(entity);
                }
            }

            a = b;
            b = Min(b + kPacketSize, end);
        }
    }
};
