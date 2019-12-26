#include "components/system.h"
#include "allocator/allocator.h"
#include "containers/hash_set.h"
#include "containers/array.h"
#include "common/find.h"
#include "common/sort.h"

static constexpr auto GuidComparator = OpComparator<Guid>();
static HashDict<Guid, i32, GuidComparator> ms_lookup = { Alloc_Pool };
static HashSet<Guid, GuidComparator> ms_initialized = { Alloc_Pool };
static Array<System> ms_systems = { Alloc_Pool };
static bool ms_needsSort;

static const System& GetSystem(Guid name)
{
    const i32* pIndex = ms_lookup.Get(name);
    ASSERT(pIndex);
    return ms_systems[*pIndex];
}

namespace SystemComparator
{
    static bool Equals(const System& lhs, const System& rhs)
    {
        return lhs.Name == rhs.Name;
    }

    static i32 Compare(const System& lhs, const System& rhs)
    {
        // can happen when resolving a dependency chain
        if (lhs.Name == rhs.Name)
        {
            return 0;
        }

        // check if lhs depends on rhs
        if (Contains(lhs.Dependencies, rhs.Name))
        {
            return 1;
        }

        // check if rhs depends on lhs
        if (Contains(rhs.Dependencies, lhs.Name))
        {
            return -1;
        }

        // check if lhs dependencies depend on rhs
        for (Guid dep : lhs.Dependencies)
        {
            i32 cmp = Compare(GetSystem(dep), rhs);
            if (cmp != 0)
            {
                return cmp;
            }
        }

        // check if rhs dependencies depend on lhs
        for (Guid dep : rhs.Dependencies)
        {
            i32 cmp = Compare(lhs, GetSystem(dep));
            if (cmp != 0)
            {
                return cmp;
            }
        }

        // no dependencies between these, don't care how they're sorted.
        return 0;
    }

    static u32 Hash(const System& sys)
    {
        return Fnv32Bytes(&sys.Name, sizeof(Guid));
    }

    static constexpr Comparator<System> Value = { Equals, Compare, Hash };
};

static void SortSystems()
{
    if (ms_needsSort)
    {
        ms_needsSort = false;

        Sort(ms_systems.begin(), ms_systems.Size(), SystemComparator::Value);
        const i32 count = ms_systems.Size();
        const System* systems = ms_systems.begin();
        for (i32 i = 0; i < count; ++i)
        {
            const Guid name = systems[i].Name;
            ms_lookup[name] = i;
        }
    }
}

static void InitSystem(const System& system)
{
    for (Guid dep : system.Dependencies)
    {
        InitSystem(GetSystem(dep));
    }
    if (ms_initialized.Add(system.Name))
    {
        system.Init();
    }
}

static void UpdateSystem(const System& system)
{
    if (ms_initialized.Contains(system.Name))
    {
        system.Update();
    }
}

static void ShutdownSystem(const System& system)
{
    if (ms_initialized.Remove(system.Name))
    {
        for (const System& other : ms_systems)
        {
            if (Contains(other.Dependencies, system.Name))
            {
                ShutdownSystem(other);
            }
        }
        system.Shutdown();
    }
}

namespace SystemRegistry
{
    void Register(System system)
    {
        if (ms_lookup.Add(system.Name, ms_systems.Size()))
        {
            ms_systems.Grow() = system;
            ms_needsSort = true;
        }
    }

    void Init()
    {
        SortSystems();
        for (const System& system : ms_systems)
        {
            InitSystem(system);
        }
    }

    void Update()
    {
        SortSystems();
        for (const System& system : ms_systems)
        {
            UpdateSystem(system);
        }
    }

    void Shutdown()
    {
        SortSystems();
        const i32 count = ms_systems.Size();
        for (i32 i = count - 1; i >= 0; --i)
        {
            ShutdownSystem(ms_systems[i]);
        }
    }
};
