#pragma once

#include "components/entity.h"
#include "components/component_id.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "threading/task.h"
#include <initializer_list>

namespace ECS
{
    enum Phase : i32
    {
        Phase_MainThread,
        Phase_MultiThread,
    };

    void SetPhase(Phase phase);

    i32 EntityCount();

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

    struct ForEachTask : ITask
    {
        ForEachTask(
            std::initializer_list<ComponentType> all,
            std::initializer_list<ComponentType> none);
        ~ForEachTask();

        void Setup();
        void Execute(i32, i32) final;
        virtual void OnEntity(Entity entity) = 0;

    private:
        Array<ComponentType> m_all;
        Array<ComponentType> m_none;
    };
};
