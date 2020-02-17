#include "components/entity_system.h"
#include "os/thread.h"
#include "components/system.h"
#include "common/find.h"
#include "common/guid.h"
#include "components/ecs.h"

struct EntitySystems final : ISystem
{
private:
    OS::RWLock m_lock;
    Array<IEntitySystem*> m_systems;
    Array<Guid> m_guids;
    Array<Array<Guid>> m_edges;
    bool m_dirty;

public:
    EntitySystems() : ISystem("IEntitySystem", { "TaskGraph" }) {}

    void Init() final
    {
        m_lock.Open();
        m_systems.Init();
        m_guids.Init();
        m_edges.Init();
        m_dirty = true;
    }

    void Update() final
    {
        OS::ReadGuard guard(m_lock);

        const i32 len = m_systems.size();
        IEntitySystem** systems = m_systems.begin();
        const Guid* guids = m_guids.begin();
        const Array<Guid>* edges = m_edges.begin();

        TaskGraph::AddVertices({ reinterpret_cast<TaskNode**>(systems), len });

        if (m_dirty)
        {
            for (i32 i = 0; i < len; ++i)
            {
                TaskGraph::ClearEdges(systems[i]);
            }

            for (i32 i = len - 1; i > 0; --i)
            {
                IEntitySystem* dst = systems[i];

                for (i32 j = i - 1; j >= 0; --j)
                {
                    IEntitySystem* src = systems[j];
                    if (IEntitySystem::Overlaps(dst, src))
                    {
                        TaskGraph::AddEdge(src, dst);
                    }
                }

                for (Guid id : edges[i])
                {
                    i32 iSrc = ::Find(guids, len, id);
                    ASSERT(iSrc != -1);
                    TaskGraph::AddEdge(systems[iSrc], dst);
                }
            }

            m_dirty = false;
        }

        TaskGraph::Evaluate();
    }

    void Shutdown() final
    {
        m_lock.LockWriter();
        m_systems.Reset();
        m_guids.Reset();
        for (Array<Guid>& edge : m_edges)
        {
            edge.Reset();
        }
        m_edges.Reset();
        m_lock.Close();
    }

    void Add(cstr name, IEntitySystem* pSystem, std::initializer_list<cstr> edges)
    {
        const Guid id = ToGuid(name);
        Array<Guid> guids = CreateArray<Guid>(Alloc_Tlsf, (i32)edges.size());
        for (cstr str : edges)
        {
            guids.PushBack(ToGuid(str));
        }

        OS::WriteGuard guard(m_lock);
        ASSERT(!m_guids.Contains(id));
        m_guids.PushBack(id);
        m_systems.PushBack(pSystem);
        m_edges.PushBack(guids);
        m_dirty = true;
    }

    void Remove(IEntitySystem* pSystem)
    {
        OS::WriteGuard guard(m_lock);
        const i32 i = m_systems.Find(pSystem);
        ASSERT(i != -1);
        m_guids.Remove(i);
        m_systems.Remove(i);
        m_edges[i].Reset();
        m_edges.Remove(i);
        m_dirty = true;
    }
};

static EntitySystems ms_system;

// ----------------------------------------------------------------------------

IEntitySystem::IEntitySystem(
    cstr name,
    std::initializer_list<cstr> edges,
    std::initializer_list<ComponentType> all,
    std::initializer_list<ComponentType> none) : TaskNode(0, 0)
{
    m_all.Init();
    m_none.Init();
    Copy(m_all, all);
    Copy(m_none, none);
    ms_system.Add(name, this, edges);
}

IEntitySystem::~IEntitySystem()
{
    ms_system.Remove(this);
    m_all.Reset();
    m_none.Reset();
}

void IEntitySystem::Execute(i32 begin, i32 end)
{
    auto result = ECS::Select(m_all, m_none);
    Slice<const Entity> entities = { result.begin(), result.size() };
    Execute(entities);
}

bool IEntitySystem::Overlaps(const IEntitySystem* pLhs, const IEntitySystem* pRhs)
{
    Slice<const ComponentType> lhs = pLhs->m_all;
    Slice<const ComponentType> rhs = pRhs->m_all;
    for (ComponentType lhsType : lhs)
    {
        for (ComponentType rhsType : rhs)
        {
            if (lhsType == rhsType)
            {
                if (lhsType.write | rhsType.write)
                {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}
