#pragma once

#include "os/thread.h"
#include "containers/hash_dict.h"
#include "containers/obj_pool.h"

template<typename K, typename V, typename Args>
struct ObjTable
{
    HashDict<K, V*> m_dict;
    ObjPool<V, Args> m_pool;

    void Init()
    {
        m_dict.Init(Alloc_Pool);
        m_pool.Init();
    }

    void Reset()
    {
        for (auto pair : m_dict)
        {
            pair.value->~V();
        }
        m_dict.Reset();
        m_pool.Reset();
    }

    V* New(Args args) { return m_pool.New(args); }
    void Delete(V* ptr) { m_pool.Delete(ptr); }

    V* Get(K key)
    {
        V* ptr = nullptr;
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
        if (m_dict.Add(key, ptr))
        {
            return ptr;
        }
        Delete(ptr);
        ptr = Get(key);
        ASSERT(ptr);
        return ptr;
    }

    bool Remove(K key)
    {
        V* ptrOut = 0;
        if (m_dict.Remove(key, ptrOut))
        {
            Delete(ptrOut);
            return true;
        }
        return false;
    }
};
