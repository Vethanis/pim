#include "components/ecs.h"
#include "containers/array.h"
#include "containers/queue.h"
#include "components/system.h"
#include "components/component_id.h"
#include "common/find.h"
#include "common/sort.h"
#include "containers/bitfield.h"
#include "containers/genid.h"

namespace ECS
{
    static constexpr i32 kSlabBytes = 64 << 10;
    static constexpr i32 kSlabAlign = 16;

    struct Slab;
    struct SlabSet;

    struct alignas(kSlabAlign) Slab
    {
        SlabSet* m_pSet;
        i32 m_length;
        i32 m_capacity;
    };

    SASSERT(alignof(Slab) == kSlabAlign);
    SASSERT((sizeof(Slab) % kSlabAlign) == 0);

    struct alignas(kSlabAlign) SlabSet
    {
        TypeFlags m_flags;
        Array<Slab*> m_slabs;
        ComponentId* m_types;
        i32* m_offsets;
        i32 m_typeCount;
        i32 m_slabCapacity;
    };

    SASSERT(alignof(SlabSet) == kSlabAlign);
    SASSERT((sizeof(SlabSet) % kSlabAlign) == 0);

    struct InSlab
    {
        SlabSet* pSet;
        Slab* pSlab;
        i32 offset;
    };

    // type registry
    static i32 ms_typeCount;
    static Guid ms_guids[kMaxTypes];
    static i32 ms_strides[kMaxTypes];

    // slot reuse
    static Queue<Entity> ms_free;

    // entity slots
    static Array<Entity> ms_entities;
    static Array<InSlab> ms_inSlabs;

    // slabs
    static Array<SlabSet*> ms_slabSets;

    static bool IsSingleThreaded();
    static Slice<const i32> GetStrides();
    static i32 SizeSum(std::initializer_list<ComponentId> types);
    static Slab* CreateSlab(SlabSet* pSet);
    static void DestroySlab(Slab* pSlab);
    static SlabSet* CreateSlabSet(std::initializer_list<ComponentId> types);
    static void DestroySlabSet(SlabSet* pSet);
    static Slice<const Entity> GetEntities(const Slab* pSlab);
    static Slice<Entity> GetEntities(Slab* pSlab);
    static void AllocateEntity(SlabSet* pSet, Entity entity);
    static void FreeEntity(Entity entity);
    static void SlabCpy(Slab* pSlab, i32 dst, i32 src);
    static Array<SlabSet*> GatherSlabSets(TypeFlags all, TypeFlags none);
    static Array<Slab*> GatherSlabs(TypeFlags all, TypeFlags none);
    static Array<Entity> GatherEntities(TypeFlags all, TypeFlags none);
    static i32 CountSlabSets(TypeFlags all, TypeFlags none);
    static i32 CountSlabs(TypeFlags all, TypeFlags none);
    static i32 CountEntities(TypeFlags all, TypeFlags none);
    static const u8* GetBytes(const Slab* pSlab);
    static u8* GetBytes(Slab* pSlab);
    static Slice<const ComponentId> GetTypes(const SlabSet* pSet);
    static Slice<const i32> GetOffsets(const SlabSet* pSet);
    //static i32 SlabToRows(Slab* pSlab, Array<Row>& rows);

    static void Init()
    {
        ms_free.Init(EAlloc_Perm, 1024);
        ms_entities.Init(EAlloc_Perm, 1024);
        ms_inSlabs.Init(EAlloc_Perm, 1024);
        ms_slabSets.Init(EAlloc_Perm, 1024);

        RegisterType(TGuidOf<Entity>(), sizeof(Entity));
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_free.Reset();
        ms_entities.Reset();
        ms_inSlabs.Reset();
        for (SlabSet* pSet : ms_slabSets)
        {
            DestroySlabSet(pSet);
        }
        ms_slabSets.Reset();
    }

    static System ms_system{ "ECS", {}, Init, Update, Shutdown, };

    ComponentId RegisterType(Guid guid, i32 sizeOf)
    {
        ASSERT(!IsNull(guid));
        ASSERT(sizeOf > 0);

        i32 i = RFind(ARGS(ms_guids), guid);
        if (i == -1)
        {
            ASSERT(IsSingleThreaded());
            ASSERT(ms_typeCount < kMaxTypes);
            i = ms_typeCount++;
            ms_guids[i] = guid;
            ms_strides[i] = sizeOf;
        }

        return { i };
    }

    i32 EntityCount() { return ms_entities.size(); }

    bool IsCurrent(Entity entity)
    {
        return entity == ms_entities[entity.GetIndex()];
    }

    Entity Create(std::initializer_list<ComponentId> components)
    {
        ASSERT(IsSingleThreaded());

        Entity entity = {};
        if (!ms_free.TryPop(entity))
        {
            entity.id = GenId(ms_entities.size(), 1);
            ms_entities.PushBack(entity);
            ms_inSlabs.PushBack({});
        }

        SlabSet* pSet = CreateSlabSet(components);
        AllocateEntity(pSet, entity);

        return entity;
    }

    bool Destroy(Entity entity)
    {
        ASSERT(IsSingleThreaded());
        const i32 i = entity.GetIndex();
        if (entity == ms_entities[i])
        {
            FreeEntity(entity);
            entity.BumpVersion();
            ms_entities[i] = entity;
            ms_free.Push(entity);
            return true;
        }
        return false;
    }

    bool HasAll(Entity entity, TypeFlags all)
    {
        if (IsCurrent(entity))
        {
            ms_inSlabs[entity.GetIndex()].pSet->m_flags.HasAll(all);
        }
        return false;
    }

    bool HasAny(Entity entity, TypeFlags any)
    {
        if (IsCurrent(entity))
        {
            ms_inSlabs[entity.GetIndex()].pSet->m_flags.HasAny(any);
        }
        return false;
    }

    bool HasNone(Entity entity, TypeFlags none)
    {
        return !HasAny(entity, none);
    }

    static Array<Slab*> GatherSlabs(TypeFlags all, TypeFlags none)
    {
        Array<Slab*> results = CreateArray<Slab*>(EAlloc_TLS);

        auto sets = ms_slabSets.AsSlice();
        for (SlabSet* pSet : sets)
        {
            const TypeFlags flags = pSet->m_flags;
            if (flags.HasAll(all) && flags.HasNone(none))
            {
                results.AppendRange(pSet->m_slabs);
            }
        }

        return results;
    }

    static i32 CountSlabs(TypeFlags all, TypeFlags none)
    {
        i32 count = 0;
        const auto sets = ms_slabSets.AsCSlice();
        for (const SlabSet* const pSet : sets)
        {
            const TypeFlags flags = pSet->m_flags;
            if (flags.HasAll(all) && flags.HasNone(none))
            {
                count += pSet->m_slabs.size();
            }
        }
        return count;
    }

    static u8* GetBytes(Slab* pSlab)
    {
        return reinterpret_cast<u8*>(pSlab + 1);
    }

    //static i32 SlabToRows(Slab* pSlab, Array<Row>& rows)
    //{
    //    ASSERT(pSlab);

    //    const i32 length = pSlab->m_length;
    //    u8* const pBytes = GetBytes(pSlab);

    //    const SlabSet* const pSet = pSlab->m_pSet;
    //    ASSERT(pSet);

    //    const i32 rowCount = pSet->m_typeCount;
    //    const Slice<const ComponentId> types = { pSet->m_types, rowCount };
    //    const Slice<const i32> offsets = { pSet->m_offsets, rowCount };
    //    const Slice<const i32> strides = { ms_strides, ms_typeCount };

    //    rows.Resize(rowCount);
    //    for (i32 iRow = 0; iRow < rowCount; ++iRow)
    //    {
    //        const ComponentId type = types[iRow];
    //        rows[iRow] = { type, strides[type.Value], pBytes + offsets[iRow] };
    //    }

    //    return length;
    //}

    static bool IsSingleThreaded() { return (task_thread_id() == 0) && (task_num_active() == 0); }

    static i32 SizeSum(std::initializer_list<ComponentId> types)
    {
        i32 typeBytes = 0;
        for (ComponentId id : types)
        {
            typeBytes += ms_strides[id.Value];
        }
        return typeBytes;
    }

    static Slab* CreateSlab(SlabSet* pSet)
    {
        Slab* pSlab = pim_tcalloc(Slab, EAlloc_Perm, kSlabBytes);
        ASSERT(pSlab);
        pSlab->m_pSet = pSet;
        pSlab->m_capacity = pSet->m_slabCapacity;
        pSlab->m_length = 0;
        pSet->m_slabs.PushBack(pSlab);
        return pSlab;
    }

    static void DestroySlab(Slab* pSlab)
    {
        if (pSlab)
        {
            SlabSet* pSet = pSlab->m_pSet;
            ASSERT(pSet);
            pSet->m_slabs.FindRemove(pSlab);
            pim_free(pSlab);

            if (pSet->m_slabs.size() == 0)
            {
                DestroySlabSet(pSet);
            }
        }
    }

    static SlabSet* CreateSlabSet(std::initializer_list<ComponentId> types)
    {
        const TypeFlags flags(types);
        {
            for (SlabSet* pSet : ms_slabSets)
            {
                if (pSet->m_flags == flags)
                {
                    return pSet;
                }
            }
        }

        const i32 typeCount = (i32)types.size() + 1;
        const i32 typeBytes = SizeSum(types) + (i32)sizeof(Entity);
        const i32 padding = (i32)sizeof(Slab) + (typeCount * kSlabAlign);
        const i32 capacity = (kSlabBytes - padding) / typeBytes;

        SlabSet* pSet = pim_tcalloc(SlabSet, EAlloc_Perm, 1);
        i32* offsets = pim_tcalloc(i32, EAlloc_Perm, typeCount);
        ComponentId* ids = pim_tcalloc(ComponentId, EAlloc_Perm, typeCount);

        ids[0] = GetId<Entity>();
        memcpy(ids + 1, types.begin(), types.size() * sizeof(ComponentId));
        Sort(ids + 1, (i32)types.size());

        i32 offset = 0;
        for (i32 i = 0; i < typeCount; ++i)
        {
            offsets[i] = offset;
            const i32 stride = ms_strides[ids[i].Value];
            offset += Align(stride * capacity, kSlabAlign);
        }

        pSet->m_flags = flags;
        pSet->m_slabs.Init();
        pSet->m_offsets = offsets;
        pSet->m_types = ids;
        pSet->m_typeCount = typeCount;
        pSet->m_slabCapacity = capacity;

        ms_slabSets.PushBack(pSet);

        return pSet;
    }

    static void DestroySlabSet(SlabSet* pSet)
    {
        if (pSet)
        {
            for (Slab* pSlab : pSet->m_slabs)
            {
                pim_free(pSlab);
            }
            pSet->m_slabs.Reset();
            pim_free(pSet->m_offsets);
            pim_free(pSet->m_types);
            pim_free(pSet);

            ms_slabSets.FindRemove(pSet);
        }
    }

    static Slice<Entity> GetEntities(Slab* pSlab)
    {
        ASSERT(pSlab);
        const i32 entitiesOffset = pSlab->m_pSet->m_offsets[0];
        Entity* pEntities = reinterpret_cast<Entity*>(GetBytes(pSlab) + entitiesOffset);
        return { pEntities, pSlab->m_length };
    }

    static void AllocateEntity(SlabSet* pSet, Entity entity)
    {
        ASSERT(pSet);

        Slab* pTarget = 0;
        for (Slab* pSlab : pSet->m_slabs)
        {
            if (pSlab->m_length < pSlab->m_capacity)
            {
                pTarget = pSlab;
                break;
            }
        }
        if (!pTarget)
        {
            pTarget = CreateSlab(pSet);
        }

        i32& length = pTarget->m_length;
        const i32 index = length++;
        Slice<Entity> entities = GetEntities(pTarget);
        entities[index] = entity;

        ms_inSlabs[entity.GetIndex()] = { pTarget->m_pSet, pTarget, index };
    }

    static void FreeEntity(Entity entity)
    {
        ASSERT(IsCurrent(entity));

        InSlab inSlab = ms_inSlabs[entity.GetIndex()];

        const i32 index = inSlab.offset;
        Slab* const pSlab = inSlab.pSlab;
        ASSERT(pSlab);

        i32 back;
        {
            back = pSlab->m_length - 1;
            ASSERT(back >= 0);
            ASSERT((u32)index <= (u32)back);

            Slice<Entity> entities = GetEntities(pSlab);
            ASSERT(entities[index] == entity);
            const Entity backEntity = entities[back];

            ms_inSlabs[backEntity.GetIndex()] = inSlab;
            ms_inSlabs[entity.GetIndex()] = { nullptr, nullptr, -1 };

            pSlab->m_length = back;
        }

        if (back == 0)
        {
            DestroySlab(pSlab);
        }
        else
        {
            SlabCpy(pSlab, index, back);
        }
    }

    static void SlabCpy(Slab* pSlab, i32 dst, i32 src)
    {
        ASSERT(pSlab);

        if (dst == src)
        {
            return;
        }

        const SlabSet* const pSet = pSlab->m_pSet;
        const i32 typeCount = pSet->m_typeCount;
        const ComponentId* types = pSet->m_types;
        const i32* const offsets = pSet->m_offsets;
        u8* const pBytes = GetBytes(pSlab);

        for (i32 i = 0; i < typeCount; ++i)
        {
            const i32 stride = ms_strides[types[i].Value];
            u8* pRow = pBytes + offsets[i];
            memcpy(pRow + stride * dst, pRow + stride * src, stride);
        }
    }

    //void OnSlabTask::SetQuery(TypeFlags all, TypeFlags none)
    //{
    //    ASSERT(IsInitOrComplete());
    //    m_all = all;
    //    m_none = none;
    //    SetRange(0, CountSlabs(all, none));
    //}

    //void OnSlabTask::Execute(i32 begin, i32 end)
    //{
    //    Array<Row> rows = CreateArray<Row>(EAlloc_TLS);
    //    Array<Slab*> slabs = GatherSlabs(m_all, m_none);

    //    begin = Max(begin, 0);
    //    end = Min(end, slabs.size());
    //    for (i32 iSlab = begin; iSlab < end; ++iSlab)
    //    {
    //        Slab* const pSlab = slabs[iSlab];
    //        const i32 length = SlabToRows(pSlab, rows);
    //        OnSlab(length, rows);
    //    }

    //    slabs.Reset();
    //    rows.Reset();
    //}

    //i32 OnSlabTask::FindRow(ComponentId type, Slice<const Row> rows)
    //{
    //    for (i32 i = 0, c = rows.size(); i < c; ++i)
    //    {
    //        if (rows[i].type == type)
    //        {
    //            return i;
    //        }
    //    }
    //    return -1;
    //}
};
