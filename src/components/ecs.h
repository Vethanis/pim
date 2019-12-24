#pragma once

#include "containers/slice.h"
#include "allocator/allocator.h"
#include "components/entity.h"
#include "components/component.h"
#include "containers/bitfield.h"

using ComponentFlags = BitField<ComponentId, ComponentId_COUNT>;

struct EntityQuery
{
    ComponentFlags All;
    ComponentFlags Any;
    ComponentFlags None;
};

namespace Ecs
{
    void Init();
    void Shutdown();

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

    template<typename K>
    inline bool Has(Entity entity) { return Has(entity, K::Id); }
    template<typename K>
    inline K& Get(Entity entity) { return *reinterpret_cast<K*>(Get(entity, K::Id)); }
    template<typename K>
    inline bool Add(Entity entity) { return Add(entity, K::Id); }
    template<typename K>
    inline bool Remove(Entity entity) { return Remove(entity, K::Id); }
};
