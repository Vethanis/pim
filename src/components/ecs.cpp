#include "components/ecs.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "os/thread.h"
#include "os/atomics.h"

namespace ECS
{
    struct Row
    {
        i32 sizeOf;
        Array<i32> versions;
        Array<u8> bytes;
    };

    static OS::RWLock ms_lock;
    static Array<Guid> ms_guids;
    static Array<Row> ms_rows;
    static Array<i32> ms_versions;
    static Queue<i32> ms_free;

    struct System final : ISystem
    {
        System() : ISystem("ECS") {}
        void Init() final
        {
            ms_lock.Open();
            ms_guids.Init();
            ms_rows.Init();
            ms_versions.Init();
            ms_free.Init();
        }
        void Update() final {}
        void Shutdown() final
        {
            ms_lock.LockWriter();

            ms_guids.Reset();
            for (Row& row : ms_rows)
            {
                row.bytes.Reset();
                row.versions.Reset();
                row.sizeOf = 0;
            }
            ms_rows.Reset();

            ms_versions.Reset();
            ms_free.Reset();

            ms_lock.Close();
        }
    };

    static System ms_system;

    static i32 size() { return ms_versions.size(); }
    static i32 capacity() { return ms_versions.capacity(); }
    static bool InRange(i32 i) { return ms_versions.InRange(i); }
    static bool IsNewer(i32 prev, i32 next) { return (next - prev) > 0; }
    static bool IsActive(i32 version) { return version & 1; }
    static bool IsActive(Entity entity) { return IsActive(entity.version); }
    static u8* _Begin(ComponentId id) { return ms_rows[id.Value].bytes.begin(); }
    static i32 _SizeOf(ComponentId id) { return ms_rows[id.Value].sizeOf; }
    static i32& _EntityVersion(i32 i) { return ms_versions[i]; }

    static i32& _ComponentVersion(Entity entity, ComponentId id)
    {
        return ms_rows[id.Value].versions[entity.index];
    }
    static bool _IsCurrent(Entity entity)
    {
        return Load(_EntityVersion(entity.index), MO_Relaxed) == entity.version;
    }
    static bool _Has(Entity entity, ComponentId id)
    {
        return Load(_ComponentVersion(entity, id), MO_Relaxed) == entity.version;
    }

    ComponentId RegisterType(Guid guid, i32 sizeOf)
    {
        ASSERT(!IsNull(guid));
        ASSERT(sizeOf > 0);

        OS::WriteGuard guard(ms_lock);
        bool added = ms_guids.FindAdd(guid);
        ASSERT(added);

        Row row = {};
        row.sizeOf = sizeOf;
        row.bytes.Init();
        row.versions.Init();
        row.bytes.Resize(size() * sizeOf);
        row.versions.Resize(size());

        const i32 i = ms_rows.PushBack(row);

        return { i };
    }

    Entity Create()
    {
        Entity entity = { 0, 0 };
        if (ms_free.TryPop(entity.index))
        {
            OS::ReadGuard guard(ms_lock);
            entity.version = 1 + Inc(ms_versions[entity.index], MO_Acquire);
            ASSERT(IsActive(entity));
            return entity;
        }

        OS::WriteGuard guard(ms_lock);

        entity.version = 1;
        entity.index = ms_versions.PushBack(entity.version);
        ASSERT(IsActive(entity));

        for (Row& row : ms_rows)
        {
            row.versions.PushBack(0);
            row.bytes.ResizeRel(row.sizeOf);
        }

        return entity;
    }

    bool Destroy(Entity entity)
    {
        ASSERT(IsActive(entity));
        bool released = false;
        {
            OS::ReadGuard guard(ms_lock);
            released = CmpExStrong(ms_versions[entity.index], entity.version, entity.version + 1, MO_Acquire);
        }
        if (released)
        {
            ms_free.Push(entity.index);
        }
        return released;
    }

    bool IsCurrent(Entity entity)
    {
        ASSERT(IsActive(entity));
        OS::ReadGuard guard(ms_lock);
        return _IsCurrent(entity);
    }

    bool Has(Entity entity, ComponentId id)
    {
        ASSERT(IsActive(entity));
        OS::ReadGuard guard(ms_lock);
        return _Has(entity, id);
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(IsActive(entity));
        OS::ReadGuard guard(ms_lock);
        i32& version = _ComponentVersion(entity, id);
        const i32 sizeOf = _SizeOf(id);
        i32 prev = Load(version, MO_Relaxed);
        if (IsNewer(prev, entity.version))
        {
            ASSERT(!IsActive(prev));
            if (CmpExStrong(version, prev, entity.version, MO_Acquire))
            {
                memset(_Begin(id) + entity.index * sizeOf, 0, sizeOf);
                return true;
            }
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        ASSERT(IsActive(entity));
        OS::ReadGuard guard(ms_lock);
        i32& version = _ComponentVersion(entity, id);
        return CmpExStrong(version, entity.version, entity.version + 1, MO_Release);
    }

    bool Get(Entity entity, ComponentId id, void* dst, i32 userSizeOf)
    {
        ASSERT(IsActive(entity));
        ASSERT(dst);
        OS::ReadGuard guard(ms_lock);
        const i32 sizeOf = _SizeOf(id);
        ASSERT(sizeOf == userSizeOf);
        if (_Has(entity, id))
        {
            memcpy(dst, _Begin(id) + entity.index * sizeOf, sizeOf);
            return true;
        }
        return false;
    }

    bool Set(Entity entity, ComponentId id, const void* src, i32 userSizeOf)
    {
        ASSERT(IsActive(entity));
        ASSERT(src);
        OS::ReadGuard guard(ms_lock);
        const i32 sizeOf = _SizeOf(id);
        ASSERT(sizeOf == userSizeOf);
        if (_Has(entity, id))
        {
            memcpy(_Begin(id) + entity.index * sizeOf, src, sizeOf);
            return true;
        }
        return false;
    }

    QueryResult ForEach(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none)
    {
        using slice_t = Slice<const ComponentType>;
        return ForEach(slice_t{ all.begin(), (i32)all.size() }, slice_t{ none.begin(), (i32)none.size() });
    }

    QueryResult ForEach(Slice<const ComponentType> all, Slice<const ComponentType> none)
    {
        OS::ReadGuard guard(ms_lock);

        const i32 len = ms_versions.size();
        const i32* const versions = ms_versions.begin();
        const Row* const rows = ms_rows.begin();
        Array<Entity> results = CreateArray<Entity>(Alloc_Linear, len);

        for (i32 i = 0; i < len; ++i)
        {
            Entity entity;
            entity.index = i;
            entity.version = Load(versions[i], MO_Relaxed);
            if (IsActive(entity))
            {
                results.PushBack(entity);
            }
        }

        for (ComponentType type : all)
        {
            const i32* const rowVersions = rows[type.id.Value].versions.begin();
            for (i32 i = results.size() - 1; i >= 0; --i)
            {
                Entity entity = results[i];
                if (Load(rowVersions[entity.index], MO_Relaxed) != entity.version)
                {
                    results.Remove(i);
                }
            }
        }

        for (ComponentType type : none)
        {
            const i32* const rowVersions = rows[type.id.Value].versions.begin();
            for (i32 i = results.size() - 1; i >= 0; --i)
            {
                Entity entity = results[i];
                if (Load(rowVersions[entity.index], MO_Relaxed) == entity.version)
                {
                    results.Remove(i);
                }
            }
        }

        return QueryResult(results.begin(), results.size());
    }
};
