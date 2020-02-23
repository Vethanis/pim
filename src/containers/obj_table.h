#pragma once

#include "os/thread.h"
#include "containers/hash_dict.h"
#include "containers/obj_pool.h"

template<typename K, typename V, typename Args>
struct ObjTable
{
    OS::Mutex m_mutex;
    HashDict<K, V*> m_dict;
    ObjPoolEx<V, Args> m_pool;

    void Init()
    {
        m_mutex.Open();
        m_dict.Init(Alloc_Perm);
        m_pool.Init();
    }

    void Reset()
    {
        m_mutex.Lock();
        for (auto pair : m_dict)
        {
            pair.value->~V();
        }
        m_dict.Reset();
        m_pool.Reset();
        m_mutex.Close();
    }

    V* New(Args args) { return m_pool.New(args); }
    void Delete(V* ptr) { m_pool.Delete(ptr); }

    V* Get(K key)
    {
        V* ptr = 0;
        OS::LockGuard guard(m_mutex);
        m_dict.Get(key, ptr);
        return ptr;
    }

    V* GetAdd(K key, Args args)
    {
        V* ptr = Get(key);
        if (ptr)
        {
            return ptr;
        }
        ptr = New(args);
        {
            OS::LockGuard guard(m_mutex);
            if (m_dict.Add(key, ptr))
            {
                return ptr;
            }
        }
        Delete(ptr);
        ptr = Get(key);
        ASSERT(ptr);
        return ptr;
    }

    bool Remove(K key)
    {
        V* ptr = 0;
        {
            OS::LockGuard guard(m_mutex);
            m_dict.Remove(key, ptr);
        }
        if (ptr)
        {
            Delete(ptr);
            return true;
        }
        return false;
    }
};
