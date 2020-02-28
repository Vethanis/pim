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
    static constexpr i32 kSlabBytes = 16 << 20;
    static constexpr i32 kSlabAlign = 16;

    struct Slab;
    struct SlabSet;

    struct alignas(kSlabAlign) Slab
    {
        SlabSet* m_pSet;
        i32 m_length;
        i32 m_capacity;
        u8 m_bytes[0];
    };

    SASSERT(alignof(Slab) == kSlabAlign);
    SASSERT((sizeof(Slab) % kSlabAlign) == 0);

    struct alignas(kSlabAlign) SlabSet
    {
        Array<Slab*> m_slabs;
        ComponentId* m_types;
        i32* m_strides;
        i32* m_offsets;
        i32 m_typeCount;
        i32 m_slabCapacity;
    };

    SASSERT(alignof(SlabSet) == kSlabAlign);
    SASSERT((sizeof(SlabSet) % kSlabAlign) == 0);

    struct InSlab
    {
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
    static Array<TypeFlags> ms_slabFlags;

    static bool IsSingleThreaded();
    static i32 SizeSum(std::initializer_list<ComponentId> types);
    static Slab* CreateSlab(SlabSet* pSet);
    static void DestroySlab(Slab* pSlab);
    static SlabSet* CreateSlabSet(std::initializer_list<ComponentId> types);
    static void DestroySlabSet(SlabSet* pSet);
    static void AllocateEntity(SlabSet* pSet, Entity entity);
    static void FreeEntity(Entity entity);
    static void SlabCpy(Slab* pSlab, i32 dst, i32 src);

    struct System final : ISystem
    {
        System() : ISystem("ECS", { "TaskSystem" }) {}
        void Init() final
        {
            ms_free.Init(Alloc_Perm, 1024);
            ms_entities.Init(Alloc_Perm, 1024);
            ms_inSlabs.Init(Alloc_Perm, 1024);
            ms_slabSets.Init(Alloc_Perm, 1024);
            ms_slabFlags.Init(Alloc_Perm, 1024);
        }
        void Update() final {}
        void Shutdown() final
        {
            ms_free.Reset();
            ms_entities.Reset();
            ms_inSlabs.Reset();
            for (SlabSet* pSet : ms_slabSets)
            {
                DestroySlabSet(pSet);
            }
            ms_slabSets.Reset();
            ms_slabFlags.Reset();
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
            ms_inSlabs.PushBack({ nullptr, -1 });
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
            entity.SetVersion(entity.GetVersion() + 1);
            ms_entities[i] = entity;
            ms_free.Push(entity);
            return true;
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        if (IsCurrent(entity))
        {
            const InSlab inSlab = ms_inSlabs[entity.GetIndex()];
            ASSERT(inSlab.pSlab);
            const SlabSet* pSet = inSlab.pSlab->m_pSet;
            ASSERT(pSet);
            return Contains(pSet->m_types, pSet->m_typeCount, id);
        }
        return false;
    }

    bool Add(Entity entity, ComponentId id)
    {
        ASSERT(IsSingleThreaded());
        ASSERT(IsActive(entity));

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
        ASSERT(IsActive(entity));
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
        ASSERT(IsActive(entity));
        const i32 iRow = id.Value;
        const i32 sizeOf = ms_strides[iRow];
        u8* const ptr = ms_rowBytes[iRow].begin() + entity.index * sizeOf;
        return (entity.version == ms_rowVersions[iRow][entity.index]) ? ptr : nullptr;
    }

    Slice<u8> GetRow(ComponentId type) { return ms_rowBytes[type.Value]; }

    void ForEachTask::SetQuery(
        std::initializer_list<ComponentId> all,
        std::initializer_list<ComponentId> none)
    {
        ASSERT(IsInitOrComplete());
        m_all.Init(Alloc_Temp);
        m_none.Init(Alloc_Temp);
        Copy(m_all, all);
        Copy(m_none, none);
        const i32 len = ms_versions.size();
        SetRange(0, len, Max(16, len / kTaskSplit));
    }

    void ForEachTask::GetQuery(
        Slice<const ComponentId>& all,
        Slice<const ComponentId>& none) const
    {
        all = m_all;
        none = m_none;
    }

    void ForEachTask::Execute(i32 begin, i32 end)
    {
        const Slice<const ComponentId> all = m_all;
        const Slice<const ComponentId> none = m_none;
        const Slice<const version_t> versions = ms_versions;

        Array<Entity> results = CreateArray<Entity>(Alloc_Task, end - begin);

        for (i32 i = begin; i < end; ++i)
        {
            const Entity entity = { i, versions[i] };
            if (IsActive(entity) && HasAll(entity, all) && HasNone(entity, none))
            {
                results.AppendBack(entity);
            }
        }

        if (results.size() > 0)
        {
            OnEntities(results);
        }

        results.Reset();
    }

    static bool IsSingleThreaded() { return (ThreadId() == 0) && (NumActiveThreads() == 0); }

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
        Slab* pSlab = (Slab*)Allocator::Calloc(Alloc_Perm, kSlabBytes);
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
            Allocator::Free(pSlab);

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
            const i32 slabIndex = ms_slabFlags.Contains(flags);
            if (slabIndex != -1)
            {
                return ms_slabSets[slabIndex];
            }
        }

        const i32 typeCount = types.size() + 1;
        const i32 typeBytes = SizeSum(types) + (i32)sizeof(Entity);
        const i32 padding = (i32)sizeof(Slab) + (typeCount * kSlabAlign);
        const i32 capacity = (kSlabBytes - padding) / typeBytes;

        SlabSet* pSet = Allocator::CallocT<SlabSet>(Alloc_Perm, 1);
        i32* offsets = Allocator::CallocT<i32>(Alloc_Perm, typeCount);
        i32* strides = Allocator::CallocT<i32>(Alloc_Perm, typeCount);
        ComponentId* ids = Allocator::CallocT<ComponentId>(Alloc_Perm, typeCount);

        ids[0] = GetId<Entity>();
        memcpy(ids + 1, types.begin(), types.size() * sizeof(ComponentId));
        Sort(ids + 1, types.size());

        i32 offset = 0;
        for (i32 i = 0; i < typeCount; ++i)
        {
            offsets[i] = offset;
            strides[i] = ms_strides[ids[i].Value];
            offset += Align(strides[i] * capacity, kSlabAlign);
        }

        pSet->m_slabs.Init();
        pSet->m_offsets = offsets;
        pSet->m_strides = strides;
        pSet->m_types = ids;
        pSet->m_typeCount = typeCount;
        pSet->m_slabCapacity = capacity;

        ms_slabSets.PushBack(pSet);
        ms_slabFlags.PushBack(flags);

        return pSet;
    }

    static void DestroySlabSet(SlabSet* pSet)
    {
        if (pSet)
        {
            for (Slab* pSlab : pSet->m_slabs)
            {
                Allocator::Free(pSlab);
            }
            pSet->m_slabs.Reset();
            Allocator::Free(pSet->m_offsets);
            Allocator::Free(pSet->m_strides);
            Allocator::Free(pSet->m_types);
            Allocator::Free(pSet);

            i32 i = ms_slabSets.Find(pSet);
            ms_slabSets.Remove(i);
            ms_slabFlags.Remove(i);
        }
    }

    static Entity* GetEntities(Slab* pSlab)
    {
        ASSERT(pSlab);
        const i32 entitiesOffset = pSlab->m_pSet->m_offsets[0];
        Entity* pEntities = reinterpret_cast<Entity*>(pSlab->m_bytes + entitiesOffset);
        return pEntities;
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
        Entity* pEntities = GetEntities(pTarget);
        pEntities[index] = entity;

        ms_inSlabs[entity.GetIndex()] = { pTarget, index };
    }

    static void FreeEntity(Entity entity)
    {
        ASSERT(IsCurrent(entity));

        InSlab inSlab = ms_inSlabs[entity.GetIndex()];

        const i32 index = inSlab.offset;
        Slab* pSlab = inSlab.pSlab;
        ASSERT(pSlab);

        i32& length = pSlab->m_length;
        ASSERT((u32)index < (u32)length);

        const i32 back = --length;
        ASSERT(back >= 0);

        Entity* pEntities = GetEntities(pSlab);
        ASSERT(pEntities[index] == entity);
        const Entity backEntity = pEntities[back];

        ms_inSlabs[backEntity.GetIndex()] = inSlab;
        ms_inSlabs[entity.GetIndex()] = { nullptr, -1 };

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
        if (dst == src)
        {
            return;
        }

        const SlabSet* pSet = pSlab->m_pSet;
        const i32 typeCount = pSet->m_typeCount;
        const i32* strides = pSet->m_strides;
        const i32* offsets = pSet->m_offsets;
        u8* pBytes = pSlab->m_bytes;

        for (i32 i = 0; i < typeCount; ++i)
        {
            const i32 stride = strides[i];
            u8* pRow = pBytes + offsets[i];
            memcpy(pRow + stride * dst, pRow + stride * src, stride);
        }
    }
};
