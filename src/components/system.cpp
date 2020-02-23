#include "components/system.h"
#include "common/find.h"
#include "containers/graph.h"

static constexpr i32 MaxSystems = 64;

static i32 ms_systemCount;
static Guid ms_names[MaxSystems];
static ISystem* ms_systems[MaxSystems];
static bool ms_hasInit[MaxSystems];
static bool ms_needsSort;
static Graph ms_graph;

static const ISystem* GetSystem(Guid name)
{
    i32 i = RFind(ms_names, ms_systemCount, name);
    ASSERT(i != -1);
    return ms_systems[i];
}

static void InitSystem(i32 i)
{
    ASSERT(!ms_hasInit[i]);
    ms_hasInit[i] = true;
    ms_systems[i]->Init();
}

static void UpdateSystem(i32 i)
{
    if (ms_hasInit[i])
    {
        ms_systems[i]->Update();
    }
}

static void ShutdownSystem(i32 i)
{
    if (ms_hasInit[i])
    {
        ms_hasInit[i] = false;
        ms_systems[i]->Shutdown();
    }
}

static void SortSystems()
{
    if (ms_needsSort)
    {
        ms_graph.Clear();
        const i32 count = ms_systemCount;
        for (i32 iDst = 0; iDst < count; ++iDst)
        {
            const i32 j = ms_graph.AddVertex();
            ASSERT(iDst == j);

            Slice<const Guid> deps = ms_systems[iDst]->GetDependencies();
            for (Guid name : deps)
            {
                const i32 iSrc = Find(ms_names, count, name);
                if (iSrc != -1)
                {
                    ms_graph.AddEdge(iSrc, iDst);
                }
            }
        }
        ms_graph.Sort();
        ASSERT(ms_graph.size() == count);
        ms_needsSort = false;
    }
}

static void Register(Guid id, ISystem* pSystem)
{
    ASSERT(!Contains(ms_names, ms_systemCount, id));
    ASSERT(ms_systemCount < MaxSystems);

    const i32 i = ms_systemCount++;
    ms_names[i] = id;
    ms_systems[i] = pSystem;
    ms_hasInit[i] = false;

    ms_needsSort = true;
}

ISystem::ISystem(cstr name, std::initializer_list<cstr> dependencies)
{
    m_dependencies.Init(Alloc_Init, (i32)dependencies.size());
    for (cstr depName : dependencies)
    {
        m_dependencies.PushBack(ToGuid(depName));
    }
    Register(ToGuid(name), this);
}

namespace Systems
{
    void Init()
    {
        ms_graph.Init(Alloc_Perm);
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            InitSystem(ms_graph[i]);
        }
    }

    void Update()
    {
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            UpdateSystem(ms_graph[i]);
        }
    }

    void Shutdown()
    {
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = count - 1; i >= 0; --i)
        {
            ShutdownSystem(ms_graph[i]);
        }
        ms_graph.Reset();
    }

    ISystem* Find(Guid id)
    {
        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            if (ms_names[i] == id)
            {
                return ms_systems[i];
            }
        }
        return nullptr;
    }
};
