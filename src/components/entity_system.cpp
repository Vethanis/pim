#include "components/entity_system.h"
#include "os/thread.h"
#include "components/taskgraph.h"
#include "components/system.h"
#include "containers/hash_dict.h"
#include "components/component_row.h"
#include "common/sort.h"

static HashDict<Guid, IEntitySystem*> ms_systems;

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
    Array<IEntitySystem*> systems = CreateArray<IEntitySystem*>(Alloc_Linear, ms_systems.size());
    for (auto pair : ms_systems)
    {
        systems.PushBack(pair.value);
    }

    Sort(systems.begin(), systems.size());

    for (IEntitySystem* pDst : systems)
    {
        TaskGraph::AddVertex(pDst);
    }

    for (i32 i = 0; i < systems.size() - 1; ++i)
    {
        IEntitySystem* lhs = systems[i];
        IEntitySystem* rhs = systems[i + 1];
        // TODO:
        // test for query overlap (one writes on anothers query)
        // if so, add edge
    }

    for (IEntitySystem* pDst : systems)
    {
        Slice<const Guid> deps = pDst->GetDeps();
        for (Guid id : deps)
        {
            IEntitySystem* pSrc = IEntitySystem::FindSystem(id);
            ASSERT(pSrc);
            TaskGraph::AddEdge(pSrc, pDst);
        }
    }

    systems.Reset();

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
    for (TypeId type : m_readableTypes)
    {
        m_readableRows.PushBack(type.GetRow()->BorrowReader());
    }
    m_writableRows.Clear();
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
}

void QueryRows::Set(Slice<const TypeId> readable, Slice<const TypeId> writable)
{
    Copy(m_readableTypes, readable);
    Copy(m_writableTypes, writable);
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

IEntitySystem::IEntitySystem(cstr name) : TaskNode()
{
    m_id = ToGuid(name);
    m_query.Init();
    ms_systems.Add(m_id, this);
}

IEntitySystem::~IEntitySystem()
{
    IEntitySystem* pRemoved = 0;
    ms_systems.Remove(m_id, pRemoved);

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

IEntitySystem* IEntitySystem::FindSystem(Guid id)
{
    IEntitySystem* pSystem = 0;
    ms_systems.Get(id, pSystem);
    return pSystem;
}
