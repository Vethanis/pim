#pragma once

#include "os/thread.h"
#include "containers/hash_dict.h"
#include "containers/obj_pool.h"

template<typename K, typename V, const Comparator<K>& cmp>
struct ObjTable
{
    using VPtr = V*;
    using TableType = ObjTable<K, V, cmp>;

    mutable OS::RWLock m_lock;
    HashDict<K, V*, cmp> m_dict;
    ObjPool<V> m_pool;

    void Init()
    {
        m_lock.Open();
        m_dict.Init(Alloc_Pool);
        m_pool.Init();
    }

    void Reset()
    {
        m_lock.LockWriter();
        for (auto pair : m_dict)
        {
            VPtr pValue = pair.Value;
            ASSERT(pValue);
            pValue->~V();
        }
        m_pool.Reset();
        m_lock.Close();
    }

    VPtr New() { return m_pool.New(); }
    void Delete(VPtr ptr) { m_pool.Delete(ptr); }

    VPtr Get(K key)
    {
        m_lock.LockReader();
        V** ppValue = m_dict.Get(key);
        m_lock.UnlockReader();
        return ppValue ? *ppValue : nullptr;
    }

    bool Add(K key, VPtr ptrIn)
    {
        ASSERT(ptrIn);
        m_lock.LockWriter();
        bool added = m_dict.Add(key, ptrIn);
        m_lock.UnlockWriter();
        return added;
    }

    bool Remove(K key, VPtr& ptrOut)
    {
        m_lock.LockWriter();
        bool removed = m_dict.Remove(key, pValue);
        m_lock.UnlockWriter();
        return removed;
    }

    HashDict<K, V*, cmp>& BeginIter()
    {
        m_lock.LockReader();
        return m_dict;
    }
    void EndIter()
    {
        m_lock.UnlockReader();
    }
};
