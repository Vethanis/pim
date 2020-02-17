#pragma once

#include "components/entity.h"
#include "components/component_id.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include "containers/array.h"
#include <initializer_list>

namespace ECS
{
    enum Phase : i32
    {
        Phase_MainThread,
        Phase_MultiThread,
    };

    void SetPhase(Phase phase);

    Entity Create();
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    bool Add(Entity entity, ComponentId type);
    bool Remove(Entity entity, ComponentId type);
    void* Get(Entity entity, ComponentId type);

    template<typename T>
    bool Add(Entity entity) { return Add(entity, GetId<T>()); }

    template<typename T>
    bool Remove(Entity entity) { return Remove(entity, GetId<T>()); }

    template<typename T>
    T* Get(Entity entity)
    {
        return (T*)Get(entity, GetId<T>());
    }

    Slice<const Entity> ForEach(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none);
};
