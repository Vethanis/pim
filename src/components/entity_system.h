#pragma once

#include "common/guid.h"
#include "components/ecs.h"
#include "threading/task.h"
#include "containers/array.h"

struct IEntitySystem : ITask
{
    IEntitySystem(cstr name);
    virtual ~IEntitySystem();

    Guid GetId() const { return m_id; }
    Slice<const Guid> GetDependencies() const { return m_dependencies; }
    void AddDependency(cstr name);
    void RmDependency(cstr name);
    void SetQuery(std::initializer_list<TypeId> all, std::initializer_list<TypeId> none = {});
    void Execute() final;
    virtual void OnEntity(Entity entity) = 0;

private:
    Guid m_id;
    Array<Guid> m_dependencies;
    Array<TypeId> m_all;
    Array<TypeId> m_none;
};
