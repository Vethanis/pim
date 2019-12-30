#include "components/system.h"
#include "allocator/allocator.h"
#include "containers/hash_set.h"
#include "containers/array.h"
#include "common/find.h"
#include "common/sort.h"

static constexpr i32 MaxSystems = 64;
static constexpr auto GuidComparator = OpComparator<Guid>();
using GuidSet = HashSet<Guid, GuidComparator>;

static i32 ms_systemCount;
static Guid ms_names[MaxSystems];
static System ms_systems[MaxSystems];
static GuidSet ms_deps[MaxSystems];
static i32 ms_order[MaxSystems];
static bool ms_hasInit[MaxSystems];
static bool ms_needsSort;

static const System& GetSystem(Guid name)
{
    i32 i = RFind(ms_names, ms_systemCount, name);
    ASSERT(i != -1);
    return ms_systems[i];
}

static void BuildSet(const System& system, GuidSet& set)
{
    if (set.Add(system.Name))
    {
        for (Guid dep : system.Dependencies)
        {
            if (set.Add(dep))
            {
                BuildSet(GetSystem(dep), set);
            }
        }
    }
}

namespace SystemComparable
{
    static i32 Compare(const i32& lhs, const i32& rhs)
    {
        if (lhs == rhs)
        {
            return 0;
        }

        const Guid lName = ms_names[lhs];
        const Guid rName = ms_names[rhs];

        const GuidSet lSet = ms_deps[lhs];
        const GuidSet rSet = ms_deps[rhs];

        if (lSet.Contains(rName))
        {
            ASSERT(!rSet.Contains(lName));
            return 1;
        }
        if (rSet.Contains(lName))
        {
            return -1;
        }
        return 0;
    }

    static constexpr Comparable<i32> Value = { Compare };
};

static void InitSystem(i32 i)
{
    ASSERT(!ms_hasInit[i]);
    ms_hasInit[i] = true;
    ms_systems[i].Init();
}

static void UpdateSystem(i32 i)
{
    if (ms_hasInit[i])
    {
        ms_systems[i].Update();
    }
}

static void ShutdownSystem(i32 i)
{
    if (ms_hasInit[i])
    {
        ms_hasInit[i] = false;
        ms_systems[i].Shutdown();
    }
}

static void SortSystems()
{
    if (ms_needsSort)
    {
        ms_needsSort = false;

        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            ms_order[i] = i;
            ms_deps[i].Clear();
            BuildSet(ms_systems[i], ms_deps[i]);
        }
        Sort(ms_order, count, SystemComparable::Value);
    }
}

namespace SystemRegistry
{
    void Register(System system)
    {
        ASSERT(!Contains(ms_names, ms_systemCount, system.Name));
        ASSERT(ms_systemCount < MaxSystems);

        const i32 i = ms_systemCount++;
        ms_names[i] = system.Name;
        ms_systems[i] = system;
        ms_deps[i].Init(Alloc_Pool);
        ms_order[i] = i;
        ms_hasInit[i] = false;

        ms_needsSort = true;
    }

    void Init()
    {
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            InitSystem(ms_order[i]);
        }
    }

    void Update()
    {
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = 0; i < count; ++i)
        {
            UpdateSystem(ms_order[i]);
        }
    }

    void Shutdown()
    {
        SortSystems();
        const i32 count = ms_systemCount;
        for (i32 i = count - 1; i >= 0; --i)
        {
            ShutdownSystem(ms_order[i]);
        }
    }
};
