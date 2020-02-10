#include "components/ecs.h"
#include "component_row.h"
#include "containers/mtqueue.h"
#include "containers/hash_set.h"
#include "components/system.h"

namespace Ecs
{
    static MtQueue<Entity> ms_free;
    static HashSet<TypeId> ms_types;
    static OS::RWLock ms_lock;
    static i32 ms_length;
    static i32 ms_capacity;
    static i32* ms_versions;

    static i32 size() { return Load(ms_length, MO_Relaxed); }
    static i32 capacity() { return Load(ms_capacity, MO_Relaxed); }
    static bool InRange(i32 i) { return (u32)i < (u32)size(); }

    bool IsCurrent(Entity entity)
    {
        OS::ReadGuard guard(ms_lock);
        return InRange(entity.index) && (Load(ms_versions[entity.index], MO_Relaxed) == entity.version);
    }

    static bool CmpExVersion(i32 index, i32& expected, i32 desired)
    {
        OS::ReadGuard guard(ms_lock);
        return InRange(index) && CmpExStrong(ms_versions[index], expected, desired, MO_Acquire);
    }

    static void Reserve(i32 newSize)
    {
        if (newSize < capacity())
        {
            return;
        }
        OS::WriteGuard guard(ms_lock);
        const i32 curCap = capacity();
        if (newSize <= curCap)
        {
            return;
        }
        const i32 newCap = Max(newSize, Max(curCap * 2, 1024));
        ms_versions = Allocator::ReallocT<i32>(Alloc_Tlsf, ms_versions, newCap);
        memset(ms_versions + curCap, 0, (newCap - curCap) * sizeof(i32));
        Store(ms_capacity, newCap);
    }

    static bool TryInsert(Entity& entity)
    {
        Reserve(size() + 3);

        OS::ReadGuard guard(ms_lock);

        i32 len = size();
        if ((len < capacity()) && CmpExStrong(ms_length, len, len + 1, MO_Acquire))
        {
            i32 prev = Exchange(ms_versions[len], 1);
            ASSERT(!prev);
            entity.index = len;
            entity.version = 1;
            return true;
        }

        return false;
    }

    Entity Create()
    {
        Entity entity = {};
        if (ms_free.TryPop(entity))
        {
            ASSERT(InRange(entity.index));
            ASSERT(entity.version & 1);

            i32 expected = entity.version - 1;
            bool current = CmpExVersion(entity.index, expected, entity.version);
            ASSERT(current);
            return entity;
        }

        u64 spins = 0;
        while (!TryInsert(entity))
        {
            OS::Spin(++spins * 100);
        }

        return entity;
    }

    bool Destroy(Entity entity)
    {
        if (CmpExVersion(entity.index, entity.version, entity.version + 1))
        {
            ASSERT(entity.version & 1);
            entity.version += 2;
            ASSERT(entity.version & 1);

            for (TypeId type : ms_types)
            {
                type.GetRow()->Remove(entity.index);
            }

            ms_free.Push(entity);
            return true;
        }
        return false;
    }

    void* Get(Entity entity, TypeId type)
    {
        if (IsCurrent(entity))
        {
            return type.GetRow()->Get(entity.index);
        }
        return nullptr;
    }

    void* Add(Entity entity, TypeId type)
    {
        if (IsCurrent(entity))
        {
            ms_types.Add(type);
            return type.GetRow()->Add(entity.index);
        }
        return nullptr;
    }

    bool Remove(Entity entity, TypeId type)
    {
        if (IsCurrent(entity))
        {
            return type.GetRow()->Remove(entity.index);
        }
        return false;
    }

    QueryResult ForEach(std::initializer_list<TypeId> all, std::initializer_list<TypeId> none)
    {
        return ForEach(all.begin(), all.size(), none.begin(), none.size());
    }

    QueryResult ForEach(const TypeId* pAll, i32 allCount, const TypeId* pNone, i32 noneCount)
    {
        Array<Entity> results = CreateArray<Entity>(Alloc_Stack, size());

        OS::ReadGuard guard(ms_lock);
        const i32* const versions = ms_versions;

        i32 i = 0;
        while (i < size())
        {
            Entity entity;
            entity.index = i;
            entity.version = Load(versions[i], MO_Relaxed);

            if (!(entity.version & 1))
            {
                goto next;
            }

            for (i32 j = 0; j < allCount; ++j)
            {
                if (!pAll[j].GetRow()->Get(i))
                {
                    goto next;
                }
            }

            for (i32 j = 0; j < noneCount; ++j)
            {
                if (pNone[j].GetRow()->Get(i))
                {
                    goto next;
                }
            }

            results.PushBack(entity);

        next:
            ++i;
        }

        return QueryResult(results.begin(), results.size());
    }

    static void Init()
    {
        ms_lock.Open();
        ms_free.Init(Alloc_Tlsf, 1024);
        ms_types.Init(Alloc_Tlsf, 1024);
        ms_length = 0;
        ms_capacity = 0;
        ms_versions = 0;

        Reserve(1024);
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        ms_lock.LockWriter();

        ms_free.Reset();

        Allocator::Free(ms_versions);
        StorePtr(ms_versions, (i32*)0);
        Store(ms_capacity, 0);
        Store(ms_length, 0);

        for (TypeId type : ms_types)
        {
            ComponentManager::ReleaseRow(type.AsData());
        }
        ms_types.Reset();

        ms_lock.Close();
    }

    DEFINE_SYSTEM("Ecs", {}, Init, Update, Shutdown);
};
