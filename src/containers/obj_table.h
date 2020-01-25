#pragma once

#include "os/thread.h"
#include "containers/hash_dict.h"
#include "containers/obj_pool.h"

template<typename K, typename V, const Comparator<K>& cmp, u32 kWidth>
struct ObjTable
{
    SASSERT((kWidth & (kWidth - 1u)) == 0u);

    static constexpr u32 kMask = kWidth - 1u;

    OS::RWLock m_locks[kWidth];
    HashDict<K, V*, cmp> m_dicts[kWidth];
    ObjPool<V> m_pool;

    void Init()
    {
        for (u32 i = 0; i < kWidth; ++i)
        {
            m_locks[i].Open();
            m_dicts[i].Init(Alloc_Pool);
        }
        m_pool.Init();
    }

    void Reset()
    {
        for (u32 i = 0; i < kWidth; ++i)
        {
            m_locks[i].Close();
            for (auto pair : m_dicts[i])
            {
                V* pValue = pair.Value;
                ASSERT(pValue);
                m_pool.Delete(pValue);
            }
            m_dicts[i].Reset();
        }
        m_pool.Reset();
    }

    V* New() { return m_pool.New(); }
    void Delete(V* ptr) { m_pool.Delete(ptr); }

    static u32 ToIndex(K key) { return cmp.Hash(key) & kMask; }

    V* Get(K key)
    {
        const u32 i = ToIndex(key);
        m_locks[i].LockReader();
        V** ppValue = m_dicts[i].Get(key);
        m_locks[i].UnlockReader();
        return ppValue ? *ppValue : nullptr;
    }

    bool Add(K key, V* pValue)
    {
        ASSERT(pValue);
        const u32 i = ToIndex(key);
        m_locks[i].LockWriter();
        bool added = m_dicts[i].Add(key, pValue);
        m_locks[i].UnlockWriter();
        return added;
    }

    bool Remove(K key)
    {
        const u32 i = ToIndex(key);

        m_locks[i].LockReader();
        V** ppValue = m_dicts[i].Get(key);
        m_locks[i].UnlockReader();

        if (ppValue)
        {
            m_locks[i].LockWriter();
            m_dicts[i].Remove(key);
            m_locks[i].UnlockWriter();

            m_pool.Delete(*ppValue);
            return true;
        }

        return false;
    }
};
