#pragma once

#include "os/thread.h"
#include "containers/hash_dict.h"
#include "containers/obj_pool.h"

template<typename K, typename V, const Comparator<K>& cmp>
struct ObjTable
{
    using TableType = ObjTable<K, V, cmp>;

    HashDict<K, V*, cmp> m_dict;
    ObjPool<V> m_pool;

    void Init()
    {
        m_dict.Init(Alloc_Pool);
        m_pool.Init();
    }

    void Reset()
    {
        Array<K> keys = CreateArray<K>(Alloc_Linear, m_dict.size());
        Array<V*> values = CreateArray<V*>(Alloc_Linear, m_dict.size());
        m_dict.GetElements(keys, values);
        m_dict.Reset();

        for (V* value : values)
        {
            value->~V();
        }

        keys.Reset();
        values.Reset();

        m_pool.Reset();
    }

    V* New() { return m_pool.New(); }
    void Delete(V* ptr) { m_pool.Delete(ptr); }

    V* Get(K key)
    {
        V* ptr = nullptr;
        m_dict.Get(key, ptr);
        return ptr;
    }

    bool Add(K key, V* ptrIn)
    {
        ASSERT(ptrIn);
        return m_dict.Add(key, ptrIn);
    }

    V* GetAdd(K key)
    {
        V* ptr = Get(key);
        if (ptr)
        {
            return ptr;
        }
        ptr = New();
        if (Add(key, ptr))
        {
            return ptr;
        }
        Delete(ptr);
        ptr = Get(key);
        ASSERT(ptr);
        return ptr;
    }

    V* Remove(K key)
    {
        V* ptrOut = 0;
        m_dict.Remove(key, ptrOut);
        return ptrOut;
    }
};
