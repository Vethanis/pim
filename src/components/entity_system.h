#pragma once

#include "components/ecs.h"
#include "components/system.h"
#include <initializer_list>

struct IEntitySystem;

struct EntitySystemTask final : ECS::ForEachTask
{
    EntitySystemTask(
        IEntitySystem* pSystem,
        std::initializer_list<ComponentType> all,
        std::initializer_list<ComponentType> none) :
        ECS::ForEachTask(all, none),
        m_system(pSystem)
    {}

    void OnEntity(Entity entity) final;

private:
    IEntitySystem* m_system;
};

struct IEntitySystem : ISystem
{
    IEntitySystem(
        cstr name,
        std::initializer_list<cstr> dependencies,
        std::initializer_list<ComponentType> all,
        std::initializer_list<ComponentType> none);
    virtual ~IEntitySystem();

    void Update() final;
    virtual void OnEntity(Entity entity) = 0;

private:
    EntitySystemTask m_task;
};

inline void EntitySystemTask::OnEntity(Entity entity)
{
    m_system->OnEntity(entity);
}
