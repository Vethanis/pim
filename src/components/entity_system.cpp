#include "components/entity_system.h"
#include "os/thread.h"
#include "components/taskgraph.h"
#include "components/system.h"

static Array<IEntitySystem*> ms_systems = CreateArray<IEntitySystem*>(Alloc_Tlsf, 0);
static OS::RWLock ms_lock = OS::CreateRWLock();

static void AddSystem(IEntitySystem* pSystem)
{
    ASSERT(pSystem);
    OS::WriteGuard guard(ms_lock);
    ms_systems.FindAdd(pSystem);
}

static void RemoveSystem(IEntitySystem* pSystem)
{
    ASSERT(pSystem);
    OS::WriteGuard guard(ms_lock);
    ms_systems.FindRemove(pSystem);
}

static void Schedule()
{
    OS::ReadGuard guard(ms_lock);

    IEntitySystem** systems = ms_systems.begin();
    const i32 count = ms_systems.size();

    Array<TaskId> ids = CreateArray<TaskId>(Alloc_Stack, count);

    for (i32 i = 0; i < count; ++i)
    {
        TaskId id = TaskGraph::Add(systems[i]);
        ids.PushBack(id);
    }

    for (i32 iDst = 0; iDst < count; ++iDst)
    {
        Slice<const Guid> deps = systems[iDst]->GetDependencies();
        for (Guid id : deps)
        {
            i32 iSrc = -1;
            for (i32 j = 0; j < count; ++j)
            {
                if (systems[j]->GetId() == id)
                {
                    ASSERT(iDst != j);
                    iSrc = j;
                    break;
                }
            }
            ASSERT(iSrc != -1);
            TaskGraph::AddDependency(ids[iSrc], ids[iDst]);
        }
    }

    ids.Reset();
}

static void Init()
{

}

static void Update()
{
    Schedule();
}

static void Shutdown()
{

}

static constexpr Guid ms_dependencies[] =
{
    ToGuid("TaskGraph"),
};

DEFINE_SYSTEM("IEntitySystem", { ARGS(ms_dependencies) }, Init, Update, Shutdown);

// ----------------------------------------------------------------------------

IEntitySystem::IEntitySystem(cstr name) : ITask()
{
    m_id = ToGuid(name);
    m_dependencies.Init(Alloc_Tlsf);
    m_all.Init(Alloc_Tlsf);
    m_none.Init(Alloc_Tlsf);

    AddSystem(this);
}

IEntitySystem::~IEntitySystem()
{
    RemoveSystem(this);
    m_all.Reset();
    m_none.Reset();
}

void IEntitySystem::AddDependency(cstr name)
{
    Guid id = ToGuid(name);
    m_dependencies.FindAdd(id);
}

void IEntitySystem::RmDependency(cstr name)
{
    Guid id = ToGuid(name);
    m_dependencies.FindRemove(id);
}

void IEntitySystem::SetQuery(std::initializer_list<TypeId> all, std::initializer_list<TypeId> none)
{
    Copy(m_all, all);
    Copy(m_none, none);
}

void IEntitySystem::Execute()
{
    for (Entity entity : Ecs::ForEach(m_all.begin(), m_all.size(), m_none.begin(), m_none.size()))
    {
        OnEntity(entity);
    }
}
