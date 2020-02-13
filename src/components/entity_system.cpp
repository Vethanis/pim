#include "components/entity_system.h"
#include "os/thread.h"
#include "components/taskgraph.h"
#include "components/system.h"
#include "containers/hash_dict.h"
#include "components/component_row.h"
#include "common/sort.h"

static Array<IEntitySystem*> ms_systems;

static void Init()
{
    ms_systems.Init(Alloc_Tlsf);
}

static void Shutdown()
{
    ms_systems.Reset();
}

static void Update()
{
    const i32 count = ms_systems.size();

    Sort(ms_systems.begin(), count, IEntitySystem::LessThan);

    for (i32 i = 0; i < count; ++i)
    {
        TaskGraph::AddVertex(ms_systems[i]);
    }

    for (i32 i = count - 1; i > 0; --i)
    {
        IEntitySystem* dst = ms_systems[i];
        for (i32 j = i - 1; j >= 0; --j)
        {
            IEntitySystem* src = ms_systems[j];
            if (IEntitySystem::Overlaps(dst, src))
            {
                TaskGraph::AddEdge(src, dst);
            }
        }
    }

    TaskGraph::Evaluate();
}

static constexpr Guid ms_dependencies[] =
{
    ToGuid("TaskGraph"),
};

DEFINE_SYSTEM("IEntitySystem", { ARGS(ms_dependencies) }, Init, Update, Shutdown);

// ----------------------------------------------------------------------------

void QueryRows::Init()
{
    m_readableTypes.Init(Alloc_Tlsf);
    m_writableTypes.Init(Alloc_Tlsf);
    m_readableRows.Init(Alloc_Tlsf);
    m_writableRows.Init(Alloc_Tlsf);
}

void QueryRows::Reset()
{
    m_readableTypes.Reset();
    m_writableTypes.Reset();
    m_readableRows.Reset();
    m_writableRows.Reset();
}

void QueryRows::Borrow()
{
    m_readableRows.Clear();
    m_writableRows.Clear();
    for (TypeId type : m_readableTypes)
    {
        m_readableRows.PushBack(type.GetRow()->BorrowReader());
    }
    for (TypeId type : m_writableTypes)
    {
        m_writableRows.PushBack(type.GetRow()->BorrowWriter());
    }
}

void QueryRows::Return()
{
    for (i32 i = m_writableTypes.size() - 1; i >= 0; --i)
    {
        TypeId type = m_writableTypes[i];
        type.GetRow()->ReturnWriter(m_writableRows[i]);
    }
    for (i32 i = m_readableTypes.size() - 1; i >= 0; --i)
    {
        TypeId type = m_readableTypes[i];
        type.GetRow()->ReturnReader(m_readableRows[i]);
    }
    m_writableRows.Clear();
    m_readableRows.Clear();
}

void QueryRows::Set(Slice<const TypeId> readable, Slice<const TypeId> writable)
{
    Copy(m_readableTypes, readable);
    Copy(m_writableTypes, writable);
}

Slice<void*> QueryRows::GetRW(TypeId type)
{
    i32 i = m_writableTypes.Find(type);
    if (i != -1)
    {
        return m_writableRows[i];
    }
    return { 0, 0 };
}

Slice<const void*> QueryRows::GetR(TypeId type) const
{
    i32 i = m_readableTypes.Find(type);
    if (i != -1)
    {
        return m_readableRows[i];
    }
    return { 0, 0 };
}

bool QueryRows::Overlaps(const QueryRows& lhs, const QueryRows& rhs)
{
    for (TypeId type : lhs.m_writableTypes)
    {
        if (rhs.m_writableTypes.Contains(type))
        {
            return true;
        }
        if (rhs.m_readableTypes.Contains(type))
        {
            return true;
        }
    }
    for (TypeId type : lhs.m_readableTypes)
    {
        if (rhs.m_writableTypes.Contains(type))
        {
            return true;
        }
    }
    return false;
}

bool QueryRows::operator<(const QueryRows& rhs) const
{
    if (m_writableRows.size() == rhs.m_writableRows.size())
    {
        return m_readableRows.size() < rhs.m_readableRows.size();
    }
    return m_writableRows.size() < rhs.m_writableRows.size();
}

// ----------------------------------------------------------------------------

IEntitySystem::IEntitySystem() : TaskNode()
{
    m_query.Init();
    ms_systems.PushBack(this);
}

IEntitySystem::~IEntitySystem()
{
    ms_systems.FindRemove(this);
    m_query.Reset();
}

void IEntitySystem::SetQuery(std::initializer_list<TypeId> readable, std::initializer_list<TypeId> writable)
{
    m_query.Set(readable, writable);
}

void IEntitySystem::Execute()
{
    m_query.Borrow();
    Execute(m_query);
    m_query.Return();
}

bool IEntitySystem::LessThan(const IEntitySystem* pLhs, const IEntitySystem* pRhs)
{
    return pLhs->m_query < pRhs->m_query;
}

bool IEntitySystem::Overlaps(const IEntitySystem* pLhs, const IEntitySystem* pRhs)
{
    return QueryRows::Overlaps(pLhs->m_query, pRhs->m_query);
}
