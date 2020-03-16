#include "components/system.h"
#include "common/find.h"
#include "common/guid.h"
#include "containers/graph.h"

static constexpr i32 MaxSystems = 64;

#define kMaxDependencies 8

typedef struct
{
    Guid guids[kMaxDependencies];
    int32_t count;
} deps_t;

static i32 ms_systemCount;
static Guid ms_names[MaxSystems];
static System ms_systems[MaxSystems];
static deps_t ms_deps[MaxSystems];
static bool ms_hasInit[MaxSystems];
static bool ms_needsSort;
static Graph ms_graph;

static void InitSystem(i32 i)
{
    ASSERT(!ms_hasInit[i]);
    ms_systems[i].Init();
    ms_hasInit[i] = true;
}

static void UpdateSystem(i32 i)
{
    ASSERT(ms_hasInit[i]);
    ms_systems[i].Update();
}

static void ShutdownSystem(i32 i)
{
    ASSERT(ms_hasInit[i]);
    ms_systems[i].Shutdown();
    ms_hasInit[i] = false;
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

            const deps_t& deps = ms_deps[iDst];
            for (int32_t d = 0; d < deps.count; ++d)
            {
                const i32 iSrc = Find(ms_names, count, deps.guids[d]);
                if (iSrc != -1)
                {
                    ms_graph.AddEdge(iSrc, iDst);
                }
                else
                {
                    ASSERT(false);
                }
            }
        }
        ms_graph.Sort();
        ASSERT(ms_graph.size() == count);
        ms_needsSort = false;
    }
}

static void Register(Guid id, const System* pSystem)
{
    ASSERT(!Contains(ms_names, ms_systemCount, id));
    ASSERT(ms_systemCount < MaxSystems);

    const i32 i = ms_systemCount++;
    ms_names[i] = id;
    ms_systems[i] = *pSystem;
    for (cstr dep : pSystem->m_dependencies)
    {
        ASSERT(dep);
        int32_t j = ms_deps[i].count++;
        ASSERT(j < kMaxDependencies);
        ms_deps[i].guids[j] = ToGuid(dep);
    }
    ms_hasInit[i] = false;

    ms_needsSort = true;
}

System::System(
    cstr name,
    std::initializer_list<cstr> dependencies,
    void(*pInit)(),
    void(*pUpdate)(),
    void(*pShutdown)())
{
    ASSERT(name);
    ASSERT(pInit);
    ASSERT(pUpdate);
    ASSERT(pShutdown);
    m_name = name;
    m_dependencies = dependencies;
    Init = pInit;
    Update = pUpdate;
    Shutdown = pShutdown;
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
};
