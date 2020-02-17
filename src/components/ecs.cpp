#include "components/ecs.h"
#include "containers/array.h"
#include "containers/mtqueue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "os/thread.h"
#include "os/atomics.h"

namespace ECS
{
    struct Row
    {
        i32 sizeOf;
        OS::RWLock lock;
        Array<i32> versions;
        Array<u8> bytes;
    };

    static constexpr i32 kInitCapacity = 256;

    static OS::RWLock ms_lock;
    static Array<i32> ms_versions;
    static Array<Row> ms_rows;
    static Array<Guid> ms_guids;
    static MtQueue<i32> ms_free;

    struct System final : ISystem
    {
        System() : ISystem("ECS") {}
        void Init() final
        {
            ms_lock.Open();
            ms_rows.Init(Alloc_Tlsf, kInitCapacity);
            ms_guids.Init(Alloc_Tlsf, kInitCapacity);
            ms_versions.Init(Alloc_Tlsf, kInitCapacity);
            ms_free.Init(Alloc_Tlsf, kInitCapacity);
            for (i32 i = 0; i < kInitCapacity; ++i)
            {
                ms_free.Push(i);
            }
        }
        void Update() final {}
        void Shutdown() final
        {
            ms_lock.LockWriter();

            for (Row& row : ms_rows)
            {
                row.lock.LockWriter();
                row.bytes.Reset();
                row.versions.Reset();
                row.sizeOf = 0;
                row.lock.Close();
            }
            ms_rows.Reset();

            ms_guids.Reset();
            ms_versions.Reset();
            ms_free.Reset();

            ms_lock.Close();
        }
    };

    static System ms_system;

    static bool IsNewer(i32 prev, i32 next) { return (next - prev) > 0; }
    static Row& GetRow(ComponentId id) { return ms_rows[id.Value]; }

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

        OS::WriteGuard guard(ms_lock);
        bool added = ms_guids.FindAdd(guid);
        ASSERT(added);

        const i32 len = ms_versions.size();
        const i32 cap = ms_versions.capacity();

        Row row = {};
        row.sizeOf = sizeOf;
        row.lock.Open();
        row.bytes.Init();
        row.versions.Init();
        row.bytes.Reserve(cap * sizeOf);
        row.versions.Reserve(cap);
        row.bytes.Resize(len * sizeOf);
        row.versions.Resize(len);

        const i32 i = ms_rows.PushBack(row);

        return { i };
    }

    bool IsCurrent(Entity entity)
    {
        ASSERT(entity.version & 1);
        OS::ReadGuard guard(ms_lock);
        return ms_versions[entity.index] == entity.version;
    }

    Entity Create()
    {
        Entity entity = { 0, 0 };
        if (ms_free.TryPop(entity.index))
        {
            OS::WriteGuard guard(ms_lock);
            entity.version = ++ms_versions[entity.index];
            ASSERT(entity.version & 1);
            return entity;
        }

        OS::WriteGuard guard(ms_lock);
        entity.version = 1;
        entity.index = ms_versions.PushBack(entity.version);
        for (Row& row : ms_rows)
        {
            OS::WriteGuard rowGuard(row.lock);
            row.versions.PushBack(0);
            row.bytes.ResizeRel(row.sizeOf);
        }

        return entity;
    }

    bool Destroy(Entity entity)
    {
        if (IsCurrent(entity))
        {
            ms_lock.LockWriter();
            if (ms_versions[entity.index] == entity.version)
            {
                ++ms_versions[entity.index];
                ms_lock.UnlockWriter();

                ms_free.Push(entity.index);
                return true;
            }
            ms_lock.UnlockWriter();
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        OS::ReadGuard guard(ms_lock);
        const Row& row = GetRow(id);
        OS::ReadGuard rowGuard(row.lock);
        return Match(entity.version, row.versions[entity.version]);
    }

    bool Add(Entity entity, ComponentId id)
    {
        if (Has(entity, id))
        {
            return false;
        }
        if (IsCurrent(entity))
        {
            OS::ReadGuard guard(ms_lock);
            Row& row = GetRow(id);
            OS::WriteGuard rowGuard(row.lock);
            const i32 sizeOf = row.sizeOf;
            i32& version = row.versions[entity.index];
            ASSERT(!(version & 1));
            if (IsNewer(version, entity.version))
            {
                version = entity.version;
                memset(row.bytes.begin() + entity.index * sizeOf, 0, sizeOf);
                return true;
            }
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        if (Has(entity, id))
        {
            OS::ReadGuard guard(ms_lock);
            Row& row = GetRow(id);
            OS::WriteGuard rowGuard(row.lock);
            if (row.versions[entity.index] == entity.version)
            {
                ++row.versions[entity.index];
                return true;
            }
        }
        return false;
    }

    bool Get(Entity entity, ComponentId id, void* dst, i32 userSizeOf)
    {
        ASSERT(dst);
        if (IsCurrent(entity))
        {
            OS::ReadGuard guard(ms_lock);
            const Row& row = GetRow(id);
            const i32 sizeOf = row.sizeOf;
            ASSERT(sizeOf == userSizeOf);
            OS::ReadGuard rowGuard(row.lock);
            if (row.versions[entity.index] == entity.version)
            {
                memcpy(dst, row.bytes.begin() + entity.index * sizeOf, sizeOf);
                return true;
            }
        }
        return false;
    }

    bool Set(Entity entity, ComponentId id, const void* src, i32 userSizeOf)
    {
        ASSERT(src);
        if (IsCurrent(entity))
        {
            OS::ReadGuard guard(ms_lock);
            Row& row = GetRow(id);
            const i32 sizeOf = row.sizeOf;
            ASSERT(sizeOf == userSizeOf);
            OS::WriteGuard rowGuard(row.lock);
            if (row.versions[entity.index] == entity.version)
            {
                memcpy(row.bytes.begin() + entity.index * sizeOf, src, sizeOf);
                return true;
            }
        }
        return false;
    }

    Selection Select(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none)
    {
        using slice_t = Slice<const ComponentType>;
        return Select(slice_t{ all.begin(), (i32)all.size() }, slice_t{ none.begin(), (i32)none.size() });
    }

    Selection Select(Slice<const ComponentType> all, Slice<const ComponentType> none)
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
            entity.version = versions[i];
            if (entity.version & 1)
            {
                results.PushBack(entity);
            }
        }

        for (ComponentType type : all)
        {
            const Row& row = rows[type.id.Value];
            OS::ReadGuard rowGuard(row.lock);
            const i32* const rowVersions = row.versions.begin();
            for (i32 i = results.size() - 1; i >= 0; --i)
            {
                Entity entity = results[i];
                if (rowVersions[entity.index] != entity.version)
                {
                    results.Remove(i);
                }
            }
        }

        for (ComponentType type : none)
        {
            const Row& row = rows[type.id.Value];
            OS::ReadGuard rowGuard(row.lock);
            const i32* const rowVersions = row.versions.begin();
            for (i32 i = results.size() - 1; i >= 0; --i)
            {
                Entity entity = results[i];
                if (rowVersions[entity.index] == entity.version)
                {
                    results.Remove(i);
                }
            }
        }

        return Selection(results.begin(), results.size());
    }

    RowBorrow::RowBorrow(ComponentType type)
    {
        ms_lock.LockReader();
        Row& row = GetRow(type.id);
        if (type.write)
        {
            row.lock.LockWriter();
        }
        else
        {
            row.lock.LockReader();
        }
        m_type = type;
        m_stride = row.sizeOf;
        m_versions = row.versions.begin();
        m_bytes = row.bytes.begin();
    }

    RowBorrow::~RowBorrow()
    {
        Row& row = GetRow(m_type.id);
        if (m_type.write)
        {
            row.lock.UnlockWriter();
        }
        else
        {
            row.lock.UnlockReader();
        }
        ms_lock.UnlockReader();
    }

    i32 RowBorrow::Has(Entity entity) const
    {
        return Match(entity.version, m_versions[entity.index]);
    }

    i32 RowBorrow::Next(const Selection& query, i32 i) const
    {
        const i32* const versions = m_versions;
        const i32 len = query.size();
        while (i < len)
        {
            if (Match(query[i].version, versions[i]))
            {
                break;
            }
            ++i;
        }
        return i;
    }

    const void* RowBorrow::GetPtrR(Entity entity) const
    {
        ASSERT(Has(entity));
        return m_bytes + entity.index * m_stride;
    }

    void* RowBorrow::GetPtrRW(Entity entity)
    {
        ASSERT(m_type.write);
        ASSERT(Has(entity));
        return m_bytes + entity.index * m_stride;
    }
};
