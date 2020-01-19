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

    void Shutdown()
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
        m_pool.Shutdown();
    }

    V* New() { return m_pool.New(); }
    void Delete(V* ptr) { m_pool.Delete(ptr); }

    static u32 ToIndex(K key) { return cmp.Hash(key) & kMask; }

    V* Get(K key)
    {
        const u32 i = ToIndex(key);
        OS::ReadGuard guard(m_locks[i]);
        V** ppValue = m_dicts[i].Get(key);
        return ppValue ? *ppValue : nullptr;
    }

    bool Add(K key, V* pValue)
    {
        ASSERT(pValue);
        const u32 i = ToIndex(key);
        OS::WriteGuard guard(m_locks[i]);
        return m_dicts[i].Add(key, pValue);
    }

    bool Remove(K key)
    {
        const u32 i = ToIndex(key);

        m_locks[i].LockWriter();
        V** ppValue = m_dicts[i].Get(key);
        m_dicts[i].Remove(key);
        m_locks[i].UnlockWriter();

        if (ppValue)
        {
            m_pool.Delete(*ppValue);
            return true;
        }
        return false;
    }
};
