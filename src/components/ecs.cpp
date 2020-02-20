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
    struct Row
    {
        Array<i32> versions;
        Array<u8> bytes;
    };

    static constexpr i32 kInitCapacity = 256;
    static constexpr i32 kMaxTypes = 256;

    static i32 ms_phase;
    static Array<i32> ms_versions;
    static MtQueue<i32> ms_free;

    static Guid ms_guids[kMaxTypes];
    static i32 ms_strides[kMaxTypes];
    static Row ms_rows[kMaxTypes];
    static i32 ms_typeCount;

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
            }

            ms_versions.Reset();
            ms_free.Reset();
            ms_typeCount = 0;
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
        i32 i = RFind(ARGS(ms_guids), guid);
        if (i == -1)
        {
            ASSERT(ms_typeCount < kMaxTypes);
            i = ms_typeCount++;
            ms_guids[i] = guid;
            ms_strides[i] = sizeOf;
            ms_rows[i].bytes.Init(Alloc_Tlsf);
            ms_rows[i].versions.Init(Alloc_Tlsf);
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
        for(i32 i = 0; i < ms_typeCount; ++i)
        {
            Array<i32>& versions = ms_rows[i].versions;
            Array<u8>& bytes = ms_rows[i].bytes;
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

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        Array<i32>& versions = ms_rows[iRow].versions;
        Array<u8>& bytes = ms_rows[iRow].bytes;

        i32 version = Load(versions[entity.index], MO_Relaxed);
        ASSERT(!(version & 1));
        if (IsNewer(version, entity.version))
        {
            if (CmpExStrong(ms_rows[id.Value].versions[entity.index], version, entity.version, MO_Acquire))
            {
                memset(bytes.begin() + entity.index * sizeOf, 0, sizeOf);
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

        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        Array<i32>& versions = ms_rows[iRow].versions;
        Array<u8>& bytes = ms_rows[iRow].bytes;

        if (Load(versions[entity.index], MO_Relaxed) == entity.version)
        {
            return bytes.begin() + entity.index * sizeOf;
        }
        return nullptr;
    }

    ForEachTask::ForEachTask(
        std::initializer_list<ComponentType> all,
        std::initializer_list<ComponentType> none) : ITask(0, 0)
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

    void ForEachTask::Execute(i32 begin, i32 end)
    {
        const Slice<const ComponentType> all = m_all;
        const Slice<const ComponentType> none = m_none;

        const i32* const versions = ms_versions.begin();
        for (i32 i = begin; i < end;)
        {
            i32 version = Load(versions[i], MO_Relaxed);
            if (version & 1)
            {
                for (ComponentType type : all)
                {
                    if (version != Load(GetRow(type.id).versions[i], MO_Relaxed))
                    {
                        goto next;
                    }
                }
                for (ComponentType type : none)
                {
                    if (version == Load(GetRow(type.id).versions[i], MO_Relaxed))
                    {
                        goto next;
                    }
                }

                Entity entity;
                entity.index = i;
                entity.version = version;

                OnEntity(entity);
            }

        next:
            ++i;
        }
    }
};
