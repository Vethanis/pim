#include "components/ecs.h"

#include "containers/array.h"
#include "containers/generational.h"
#include "common/chunk_allocator.h"

// ----------------------------------------------------------------------------

#include "components/all.h"

#define xsizeof(x) sizeof(x),
#define nameof(x) #x,
#define guidof(x) ToGuid(#x),

static constexpr i32 ComponentSizes[] = 
{
    COMPONENTS_COMMON(xsizeof)
    COMPONENTS_TRANSFORM(xsizeof)
    COMPONENTS_RENDERING(xsizeof)
    COMPONENTS_PHYSICS(xsizeof)
    COMPONENTS_AUDIO(xsizeof)
};

static constexpr cstr ComponentNames[] = 
{
    COMPONENTS_COMMON(nameof)
    COMPONENTS_TRANSFORM(nameof)
    COMPONENTS_RENDERING(nameof)
    COMPONENTS_PHYSICS(nameof)
    COMPONENTS_AUDIO(nameof)
};

static constexpr Guid ComponentGuids[] =
{
    COMPONENTS_COMMON(guidof)
    COMPONENTS_TRANSFORM(guidof)
    COMPONENTS_RENDERING(guidof)
    COMPONENTS_PHYSICS(guidof)
    COMPONENTS_AUDIO(guidof)
};

#undef xsizeof
#undef nameof
#undef guidof

SASSERT(NELEM(ComponentSizes) == ComponentId_COUNT);
SASSERT(NELEM(ComponentNames) == ComponentId_COUNT);
SASSERT(NELEM(ComponentGuids) == ComponentId_COUNT);

static constexpr i32 SizeOf(ComponentId id) { return ComponentSizes[id]; }
static constexpr cstrc NameOf(ComponentId id) { return ComponentNames[id]; }
static constexpr Guid GuidOf(ComponentId id) { return ComponentGuids[id]; }

// ----------------------------------------------------------------------------

static IdAllocator ms_ids;
static Array<ComponentFlags> ms_flags;
static Array<void*> ms_ptrs[ComponentId_COUNT];
static ChunkAllocator ms_allocators[ComponentId_COUNT];

static void* Allocate(i32 id, i32 index)
{
    Array<void*>& arr = ms_ptrs[id];
    const i32 size = arr.Size();
    if (size <= index)
    {
        arr.Resize(index + 1);
        void** ptrs = arr.begin();
        for (i32 i = size; i <= index; ++i)
        {
            ptrs[i] = nullptr;
        }
    }

    ASSERT(!arr[index]);

    void* ptr = ms_allocators[id].Allocate();
    arr[index] = ptr;

    return ptr;
}

static void Free(i32 id, i32 index)
{
    void* ptr = ms_ptrs[id][index];
    ms_ptrs[id][index] = nullptr;
    ASSERT(ptr);
    ms_allocators[id].Free(ptr);
}

// ----------------------------------------------------------------------------

static void SearchAll(
    ComponentFlags all,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        if ((flags[i] & all) == all)
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchAny(
    ComponentFlags any,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        if ((flags[i] & any).Any())
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchNone(
    ComponentFlags none,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        if ((flags[i] & none).None())
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchAllAny(
    ComponentFlags all,
    ComponentFlags any,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        bool hasAll = (flags[i] & all) == all;
        bool hasAny = (flags[i] & any).Any();
        if (hasAll && hasAny)
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchAllNone(
    ComponentFlags all,
    ComponentFlags none,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        bool hasAll = (flags[i] & all) == all;
        bool hasNone = (flags[i] & none).None();
        if (hasAll && hasNone)
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchAnyNone(
    ComponentFlags any,
    ComponentFlags none,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        bool hasAny = (flags[i] & any).Any();
        bool hasNone = (flags[i] & none).None();
        if (hasAny && hasNone)
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

static void SearchAllAnyNone(
    ComponentFlags all,
    ComponentFlags any,
    ComponentFlags none,
    Array<Entity>& storage)
{
    const i32 count = ms_ids.m_versions.Size();
    const u16* versions = ms_ids.m_versions.begin();
    const ComponentFlags* flags = ms_flags.begin();
    for (i32 i = 0; i < count; ++i)
    {
        bool hasAll = (flags[i] & all) == all;
        bool hasAny = (flags[i] & any).Any();
        bool hasNone = (flags[i] & none).None();
        if (hasAll && hasAny && hasNone)
        {
            Entity entity = {};
            entity.version = versions[i];
            entity.index = (u16)i;
            storage.Grow() = entity;
        }
    }
}

// ----------------------------------------------------------------------------

namespace Ecs
{
    void Init()
    {
        AllocType allocator = Alloc_Pool;
        ms_ids.Init(allocator);
        ms_flags.Init(allocator);
        for (i32 i = 0; i < ComponentId_COUNT; ++i)
        {
            ms_ptrs[i].Init(allocator);
            ms_allocators[i].Init(allocator, ComponentSizes[i]);
        }
    }

    void Shutdown()
    {
        ms_ids.Reset();
        ms_flags.Reset();
        for (i32 i = 0; i < ComponentId_COUNT; ++i)
        {
            ms_ptrs[i].Reset();
            ms_allocators[i].Reset();
        }
    }

    bool IsCurrent(Entity entity)
    {
        return ms_ids.IsCurrent(entity.version, entity.index);
    }

    Entity Create()
    {
        Entity entity = {};
        if (ms_ids.Create(entity.version, entity.index))
        {
            ms_flags.Grow() = {};
        }
        return entity;
    }

    Entity Create(std::initializer_list<ComponentId> ids)
    {
        Entity entity = Create();
        for (ComponentId id : ids)
        {
            Add(entity, id);
        }
        return entity;
    }

    bool Destroy(Entity entity)
    {
        if (ms_ids.Destroy(entity.version, entity.index))
        {
            ComponentFlags flags = ms_flags[entity.index];
            ms_flags[entity.index] = {};
            for (i32 i = 0; i < ComponentId_COUNT; ++i)
            {
                if (flags.Has(i))
                {
                    Free(i, entity.index);
                }
            }
            return true;
        }
        return false;
    }

    bool Has(Entity entity, ComponentId id)
    {
        ASSERT(IsCurrent(entity));
        return ms_flags[entity.index].Has(id);
    }

    void* Get(Entity entity, ComponentId id)
    {
        ASSERT(Has(entity, id));
        return ms_ptrs[id][entity.index];
    }

    bool Add(Entity entity, ComponentId id)
    {
        if (!Has(entity, id))
        {
            ms_flags[entity.index].Set(id);
            Allocate(id, entity.index);
            return true;
        }
        return false;
    }

    bool Remove(Entity entity, ComponentId id)
    {
        if (Has(entity, id))
        {
            ms_flags[entity.index].UnSet(id);
            Free(id, entity.index);
            return true;
        }
        return false;
    }

    bool HasAll(Entity entity, ComponentFlags flags)
    {
        ASSERT(IsCurrent(entity));
        return (ms_flags[entity.index] & flags) == flags;
    }

    bool HasAny(Entity entity, ComponentFlags flags)
    {
        ASSERT(IsCurrent(entity));
        return (ms_flags[entity.index] & flags).Any();
    }

    bool HasNone(Entity entity, ComponentFlags flags)
    {
        ASSERT(IsCurrent(entity));
        return (ms_flags[entity.index] & flags).None();
    }

    bool Add(Entity entity, std::initializer_list<ComponentId> ids)
    {
        ASSERT(IsCurrent(entity));
        bool added = false;
        for (ComponentId id : ids)
        {
            added |= Add(entity, id);
        }
        return added;
    }

    bool Remove(Entity entity, std::initializer_list<ComponentId> ids)
    {
        ASSERT(IsCurrent(entity));
        bool removed = false;
        for (ComponentId id : ids)
        {
            removed |= Remove(entity, id);
        }
        return removed;
    }

    QueryResult Search(EntityQuery query)
    {
        Array<Entity> storage = {};
        storage.Init(Alloc_Linear);

        i32 x = 0;
        x |= query.All.Any() ? 1 << 0 : 0;
        x |= query.Any.Any() ? 1 << 1 : 0;
        x |= query.None.Any() ? 1 << 2 : 0;

        switch (x)
        {
        default:
        case 0:
            break;
        case 1:
            SearchAll(query.All, storage);
            break;
        case 2:
            SearchAny(query.Any, storage);
            break;
        case 4:
            SearchNone(query.None, storage);
            break;
        case (1 | 2):
            SearchAllAny(query.All, query.Any, storage);
            break;
        case (1 | 4):
            SearchAllNone(query.All, query.None, storage);
            break;
        case (2 | 4):
            SearchAnyNone(query.Any, query.None, storage);
            break;
        case (1 | 2 | 4):
            SearchAllAnyNone(query.All, query.Any, query.None, storage);
            break;
        }

        return { storage.begin(), storage.Size() };
    }

    void Add(EntityQuery query, std::initializer_list<ComponentId> ids)
    {
        for (Entity entity : Search(query))
        {
            Add(entity, ids);
        }
    }

    void Remove(EntityQuery query, std::initializer_list<ComponentId> ids)
    {
        for (Entity entity : Search(query))
        {
            Remove(entity, ids);
        }
    }

    void Destroy(EntityQuery query)
    {
        for (Entity entity : Search(query))
        {
            Destroy(entity);
        }
    }
};
