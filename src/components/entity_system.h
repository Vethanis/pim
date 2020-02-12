#pragma once

#include "common/guid.h"
#include "components/ecs.h"
#include "components/taskgraph.h"

struct QueryRows
{
    Array<TypeId> m_readableTypes;
    Array<Slice<const void*>> m_readableRows;
    Array<TypeId> m_writableTypes;
    Array<Slice<void*>> m_writableRows;

    void Init();
    void Reset();
    void Borrow();
    void Return();

    void Set(Slice<const TypeId> readable, Slice<const TypeId> writable);

    bool operator<(const QueryRows& rhs) const;
};

struct IEntitySystem : TaskNode
{
    IEntitySystem(cstr name);
    virtual ~IEntitySystem();

    Guid GetId() const { return m_id; }
    bool AddDep(Guid id) { return m_depIds.FindAdd(id); }
    bool AddDep(cstr name) { return AddDep(ToGuid(name)); }
    bool RmDep(Guid id) { return m_depIds.FindRemove(id); }
    bool RmDep(cstr name) { return RmDep(ToGuid(name)); }
    Slice<const Guid> GetDeps() const { return m_depIds; }

    void SetQuery(std::initializer_list<TypeId> readable, std::initializer_list<TypeId> writable);
    void Execute() final;
    virtual void Execute(QueryRows& rows) = 0;

    static IEntitySystem* FindSystem(Guid id);
    static IEntitySystem* FindSystem(cstr name) { return FindSystem(ToGuid(name)); }

private:
    Guid m_id;
    Array<Guid> m_depIds;
    QueryRows m_query;
};
