#pragma once

#include "common/guid.h"
#include "components/ecs.h"
#include "components/taskgraph.h"

struct IEntitySystem : TaskNode
{
    IEntitySystem(cstr name);
    virtual ~IEntitySystem();

    bool AddEdge(IEntitySystem* pSrc) { return m_deps.FindAdd(pSrc); }
    bool RmEdge(IEntitySystem* pSrc) { return m_deps.FindRemove(pSrc); }
    Slice<IEntitySystem*> GetEdges() { return m_deps; }

    void SetQuery(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none);

    void Execute() final;
    virtual void Execute(Slice<const Entity> entities) = 0;

    static bool Overlaps(const IEntitySystem* pLhs, const IEntitySystem* pRhs);
    static IEntitySystem* FindSystem(Guid id);

private:
    Array<ComponentType> m_all;
    Array<ComponentType> m_none;
    Array<IEntitySystem*> m_deps;
};
