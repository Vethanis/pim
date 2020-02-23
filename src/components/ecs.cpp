#include "components/ecs.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "common/find.h"

namespace ECS
{
    static Array<version_t> ms_versions;
    static Queue<index_t> ms_free;

    static i32 ms_typeCount;
    static Guid ms_guids[kMaxTypes];
    static i32 ms_strides[kMaxTypes];
    static Array<version_t> ms_rowVersions[kMaxTypes];
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
            ms_versions.Init(Alloc_Perm, 1024);
            ms_free.Init(Alloc_Perm, 1024);
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
            ms_rowBytes[i].Init(Alloc_Perm);
            ms_rowVersions[i].Init(Alloc_Perm);
        }

        return { i };
    }

    i32 EntityCount() { return ms_versions.size(); }

    bool IsCurrent(Entity entity)
    {
        ASSERT(entity.version & 1);
        return entity.version == ms_versions[entity.index];
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
            Array<version_t>& versions = ms_rowVersions[i];
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
        if (entity.version == ms_versions[entity.index])
        {
            ++ms_versions[entity.index];
            ms_free.Push(entity.index);
            return true;
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        return entity.version == ms_rowVersions[id.Value][entity.index];
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(IsSingleThreaded());
        ASSERT(entity.version & 1);

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* const ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        version_t& rVersion = ms_rowVersions[iRow][entity.index];

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
        version_t& rVersion = ms_rowVersions[id.Value][entity.index];
        if (entity.version == rVersion)
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
        u8* const ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        return (entity.version == ms_rowVersions[iRow][entity.index]) ? ptr : nullptr;
    }

    Slice<u8> GetRow(ComponentId type)
    {
        return ms_rowBytes[type.Value];
    }

    void ForEachTask::SetQuery(
        std::initializer_list<ComponentId> all,
        std::initializer_list<ComponentId> none)
    {
        m_all.Init(Alloc_Temp);
        m_none.Init(Alloc_Temp);
        Copy(m_all, all);
        Copy(m_none, none);
        SetRange(0, ms_versions.size());
    }

    void ForEachTask::Execute(i32 begin, i32 end)
    {
        const Slice<const ComponentId> all = m_all;
        const Slice<const ComponentId> none = m_none;
        const Slice<const version_t> versions = ms_versions;
        const Slice<const Array<version_t>> rows = { ms_rowVersions, kMaxTypes };

        Array<Entity> results = CreateArray<Entity>(Alloc_Task, end - begin);

        for(i32 ei = begin; ei < end;)
        {
            const version_t ev = versions[ei];
            if (ev & 1)
            {
                for (ComponentId id : all)
                {
                    const version_t cv = rows[id.Value][ei];
                    if (ev != cv)
                    {
                        goto next;
                    }
                }
                for (ComponentId id : none)
                {
                    const version_t cv = rows[id.Value][ei];
                    if (ev == cv)
                    {
                        goto next;
                    }
                }
                results.AppendBack({ ei, ev });
            }
        next:
            ++ei;
        }

        if (results.size() > 0)
        {
            OnEntities(results);
        }

        results.Reset();
    }
};
