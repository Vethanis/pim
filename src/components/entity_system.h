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

    Slice<void*> GetRW(TypeId type);
    Slice<const void*> GetR(TypeId type) const;

    template<typename T>
    Slice<T*> GetRW()
    {
        Slice<void*> row = GetRW(TypeOf<T>());
        return { (T*)row.begin(), row.size() };
    }

    template<typename T>
    Slice<const T*> GetR()
    {
        Slice<const void*> row = GetR(TypeOf<T>());
        return { (const T*)row.begin(), row.size() };
    }

    static bool Overlaps(const QueryRows& lhs, const QueryRows& rhs);
    bool operator<(const QueryRows& rhs) const;
};

struct IEntitySystem : TaskNode
{
    IEntitySystem();
    virtual ~IEntitySystem();

    void SetQuery(std::initializer_list<TypeId> readable, std::initializer_list<TypeId> writable);
    void Execute() final;
    virtual void Execute(QueryRows& rows) = 0;

    static bool LessThan(const IEntitySystem* pLhs, const IEntitySystem* pRhs);
    static bool Overlaps(const IEntitySystem* pLhs, const IEntitySystem* pRhs);

private:
    QueryRows m_query;
};
