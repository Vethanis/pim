#include "components/ecs.h"

#include "containers/array.h"
#include "containers/generational.h"

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

struct Archetype
{

};

struct Chunk
{

};

struct Entities
{
    Array<u16> m_freelist;

    Array<u16> m_version;           // version of the entity slot
    Array<Chunk*> m_chunk;          // chunk the entity belongs to
    Array<u16> m_chunkIndex;        // index of the entity in the chunk
    Array<Archetype*> m_archetype;  // archetype the entity belongs to
    Array<ComponentFlags> m_flags;  // which components the entity has
};

// ----------------------------------------------------------------------------

namespace Ecs
{
    void Init()
    {

    }

    void Shutdown()
    {

    }

    Entity Create();
    Entity Create(std::initializer_list<ComponentId> ids);
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    bool Has(Entity entity, ComponentId id);
    void* Get(Entity entity, ComponentId id);
    bool Add(Entity entity, ComponentId id);
    bool Remove(Entity entity, ComponentId id);

    struct QueryResult
    {
        Entity* m_ptr; i32 m_len;
        inline QueryResult(Entity* ptr, i32 len) : m_ptr(ptr), m_len(len) {}
        ~QueryResult() { Allocator::Free(m_ptr); }
        inline const Entity* begin() const { return m_ptr; }
        inline const Entity* end() const { return m_ptr + m_len; }
        inline i32 size() const { return m_len; }
    };

    QueryResult Search(EntityQuery query);
    void Destroy(EntityQuery query);
};
