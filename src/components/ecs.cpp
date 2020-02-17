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
        Array<i32> versions;
        Array<u8> bytes;
    };

    static constexpr i32 kInitCapacity = 256;

    static i32 ms_phase;
    static Array<i32> ms_versions;
    static Array<Row> ms_rows;
    static Array<Guid> ms_guids;
    static MtQueue<i32> ms_free;

    void SetPhase(Phase phase)
    {
        Store(ms_phase, phase);
    }

    struct System final : ISystem
    {
        System() : ISystem("ECS") {}
        void Init() final
        {
            SetPhase(Phase_MainThread);
            ms_rows.Init(Alloc_Tlsf, kInitCapacity);
            ms_guids.Init(Alloc_Tlsf, kInitCapacity);
            ms_versions.Init(Alloc_Tlsf, kInitCapacity);
            ms_free.Init(Alloc_Tlsf, kInitCapacity);
        }
        void Update() final
        {
            SetPhase(Phase_MainThread);
        }
        void Shutdown() final
        {
            SetPhase(Phase_MainThread);
            for (Row& row : ms_rows)
            {
                row.bytes.Reset();
                row.versions.Reset();
                row.sizeOf = 0;
            }
            ms_rows.Reset();

            ms_guids.Reset();
            ms_versions.Reset();
            ms_free.Reset();
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

        ASSERT(Load(ms_phase) == Phase_MainThread);
        bool added = ms_guids.FindAdd(guid);
        ASSERT(added);

        const i32 len = ms_versions.size();
        const i32 cap = ms_versions.capacity();

        Row row = {};
        row.sizeOf = sizeOf;
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
        return ms_versions[entity.index] == entity.version;
    }

    Entity Create()
    {
        ASSERT(Load(ms_phase) == Phase_MainThread);

        Entity entity = { 0, 0 };
        if (ms_free.TryPop(entity.index))
        {
            entity.version = 1 + Inc(ms_versions[entity.index]);
            ASSERT(entity.version & 1);
            return entity;
        }

        entity.version = 1;
        entity.index = ms_versions.PushBack(entity.version);
        for (Row& row : ms_rows)
        {
            row.versions.PushBack(0);
            row.bytes.ResizeRel(row.sizeOf);
        }

        return entity;
    }

    bool Destroy(Entity entity)
    {
        ASSERT(Load(ms_phase) == Phase_MainThread);
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
        const Row& row = GetRow(id);
        return entity.version == Load(row.versions[entity.index], MO_Relaxed);
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        Row& row = GetRow(id);
        const i32 sizeOf = row.sizeOf;
        i32 version = Load(row.versions[entity.index], MO_Relaxed);
        ASSERT(!(version & 1));
        if (IsNewer(version, entity.version))
        {
            if (CmpExStrong(row.versions[entity.index], version, entity.version, MO_Acquire))
            {
                memset(row.bytes.begin() + entity.index * sizeOf, 0, sizeOf);
                return true;
            }
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        Row& row = GetRow(id);
        return CmpExStrong(row.versions[entity.index], entity.version, entity.version + 1, MO_Release);
    }

    void* Get(Entity entity, ComponentId id)
    {
        ASSERT(entity.version & 1);
        Row& row = GetRow(id);
        const i32 sizeOf = row.sizeOf;
        if (Load(row.versions[entity.index], MO_Relaxed) == entity.version)
        {
            return row.bytes.begin() + entity.index * sizeOf;
        }
        return nullptr;
    }

    Slice<const Entity> ForEach(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none)
    {
        const i32 len = ms_versions.size();
        const i32* const versions = ms_versions.begin();

        Entity* entities = Allocator::AllocT<Entity>(Alloc_Linear, len);
        i32 ct = 0;

        for (i32 i = 0; i < len; ++i)
        {
            i32 version = Load(versions[i], MO_Relaxed);
            if (version & 1)
            {
                entities[ct++] = { i, version };
            }
        }

        for (ComponentType type : all)
        {
            const i32* const compVersions = GetRow(type.id).versions.begin();
            for (i32 i = ct - 1; i >= 0; --i)
            {
                Entity entity = entities[i];
                i32 version = Load(compVersions[entity.index], MO_Relaxed);
                if (entity.version != version)
                {
                    entities[i] = entities[--ct];
                }
            }
        }

        for (ComponentType type : none)
        {
            const i32* const compVersions = GetRow(type.id).versions.begin();
            for (i32 i = ct - 1; i >= 0; --i)
            {
                Entity entity = entities[i];
                i32 version = Load(compVersions[entity.index], MO_Relaxed);
                if (entity.version == version)
                {
                    entities[i] = entities[--ct];
                }
            }
        }

        return { entities, ct };
    }
};
