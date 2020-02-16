#include "components/entity_system.h"
#include "os/thread.h"
#include "components/taskgraph.h"
#include "components/system.h"

struct Scheduler final : ISystem
{
private:
    OS::RWLock m_lock;
    Array<IEntitySystem*> m_systems;
    Array<Guid> m_guids;

public:
    Scheduler() : ISystem("IEntitySystem", { "TaskGraph" }) {}

    void Init() final
    {
        m_lock.Open();
        m_systems.Init();
        m_guids.Init();
    }

    void Update() final
    {
        OS::ReadGuard guard(m_lock);

        for (IEntitySystem* pSystem : m_systems)
        {
            TaskGraph::AddVertex(pSystem);
        }

        for (i32 i = m_systems.size() - 1; i > 0; --i)
        {
            IEntitySystem* dst = m_systems[i];

            for (i32 j = i - 1; j >= 0; --j)
            {
                IEntitySystem* src = m_systems[j];
                if (IEntitySystem::Overlaps(dst, src))
                {
                    TaskGraph::AddEdge(src, dst);
                }
            }

            for (IEntitySystem* src : dst->GetEdges())
            {
                TaskGraph::AddEdge(src, dst);
            }
        }

        TaskGraph::Evaluate();
    }

    void Shutdown() final
    {
        m_lock.LockWriter();
        m_systems.Reset();
        m_guids.Reset();
        m_lock.Close();
    }

    void Register(cstr name, IEntitySystem* pSystem)
    {
        const Guid id = ToGuid(name);
        OS::WriteGuard guard(m_lock);
        ASSERT(!m_guids.Contains(id));
        m_guids.PushBack(id);
        m_systems.PushBack(pSystem);
    }

    void Remove(IEntitySystem* pSystem)
    {
        OS::WriteGuard guard(m_lock);
        const i32 i = m_systems.Find(pSystem);
        ASSERT(i != -1);
        m_guids.Remove(i);
        m_systems.Remove(i);
    }

    IEntitySystem* Find(Guid id)
    {
        OS::ReadGuard guard(m_lock);
        const i32 i = m_guids.Find(id);
        return (i == -1) ? nullptr : m_systems[i];
    }
};

static Scheduler ms_system;

// ----------------------------------------------------------------------------

IEntitySystem::IEntitySystem(cstr name) : TaskNode()
{
    m_all.Init();
    m_none.Init();
    m_deps.Init();
    ms_system.Register(name, this);
}

IEntitySystem::~IEntitySystem()
{
    ms_system.Remove(this);
    m_all.Reset();
    m_none.Reset();
    m_deps.Reset();
}

void IEntitySystem::SetQuery(std::initializer_list<ComponentType> all, std::initializer_list<ComponentType> none)
{
    Copy(m_all, all);
    Copy(m_none, none);
}

void IEntitySystem::Execute()
{
    ECS::QueryResult result = ECS::ForEach(m_all, m_none);
    Slice<const Entity> entities = { result.begin(), result.size() };
    Execute(entities);
}

bool IEntitySystem::Overlaps(const IEntitySystem* pLhs, const IEntitySystem* pRhs)
{
    for (ComponentType lhsType : pLhs->m_all)
    {
        for (ComponentType rhsType : pRhs->m_all)
        {
            if (lhsType == rhsType)
            {
                if (lhsType.write | rhsType.write)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

IEntitySystem* IEntitySystem::FindSystem(Guid id)
{
    return ms_system.Find(id);
}
