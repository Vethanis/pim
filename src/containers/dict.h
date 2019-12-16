#pragma once

#include "containers/array.h"

template<typename K, typename V>
struct Dict
{
    Array<u32> m_hashes;
    Array<K> m_keys;
    Array<V> m_values;

    inline static constexpr u32 GetHash(K key) { return Fnv32Bytes(&key, sizeof(K)); }

    inline i32 Size() const { return m_keys.Size(); }
    inline i32 Capacity() const { return m_keys.Capacity(); }
    inline bool IsEmpty() const { return m_keys.IsEmpty(); }
    inline bool IsFull() const { return m_keys.IsFull(); }

    inline Slice<K> Keys() { return m_keys; }
    inline Slice<const K> Keys() const { return m_keys; }
    inline Slice<V> Values() { return m_values; }
    inline Slice<const V> Values() const { return m_values; }

    inline void Init(AllocType allocType)
    {
        m_hashes.Init(allocType);
        m_keys.Init(allocType);
        m_values.Init(allocType);
    }
    inline void Reset()
    {
        m_hashes.Reset();
        m_keys.Reset();
        m_values.Reset();
    }
    inline void Clear()
    {
        m_hashes.Clear();
        m_keys.Clear();
        m_values.Clear();
    }

    inline i32 Find(K key, u32 hash) const
    {
        const i32 count = m_hashes.Size();
        const u32* const hashes = m_hashes.begin();
        const K* const keys = m_keys.begin();
        for (i32 i = count - 1; i >= 0; --i)
        {
            if (hashes[i] == hash)
            {
                if (key == keys[i])
                {
                    return i;
                }
            }
        }
        return -1;
    }
    inline i32 Find(K key) const
    {
        return Find(key, GetHash(key));
    }

    inline bool Contains(K key, u32 hash) const { return Find(key, hash) != -1; }
    inline bool Contains(K key) const { return Contains(key, GetHash(key)); }

    inline const V* Get(K key, u32 hash) const
    {
        const i32 i = Find(key, hash);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline const V* Get(K key) const { return Get(key, GetHash(key)); }
    inline const V GetOrNull(K key) const
    {
        const V* ptr = Get(key);
        return ptr ? *ptr : nullptr;
    }

    inline V* Get(K key, u32 hash)
    {
        const i32 i = Find(key, hash);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline V* Get(K key) { return Get(key, GetHash(key)); }
    inline V GetOrNull(K key)
    {
        V* ptr = Get(key);
        return ptr ? *ptr : nullptr;
    }

    inline bool Remove(K key, u32 hash)
    {
        const i32 i = Find(key, hash);
        if (i != -1)
        {
            m_hashes.Remove(i);
            m_keys.Remove(i);
            m_values.Remove(i);
            return true;
        }
        return false;
    }
    inline bool Remove(K key) { return Remove(key, GetHash(key)); }

    inline V& operator[](K key)
    {
        const u32 hash = GetHash(key);
        const i32 i = Find(key, hash);
        if (i != -1)
        {
            return m_values[i];
        }
        else
        {
            m_hashes.Grow() = hash;
            m_keys.Grow() = key;
            return m_values.Grow();
        }
    }
};

template<u32 t_width, typename K, typename V>
struct DictTable
{
    static constexpr u32 Width = t_width;
    static constexpr u32 Mask = t_width - 1u;

    SASSERT((t_width & (t_width - 1u)) == 0);

    inline static constexpr u32 GetHash(K key)
    {
        return Dict<K, V>::GetHash(key);
    }
    inline static u32 GetSlot(u32 hash)
    {
        return hash & Mask;
    }

    Dict<K, V> m_dicts[Width];

    inline void Init(AllocType allocType)
    {
        for (auto& dict : m_dicts)
        {
            dict.Init(allocType);
        }
    }
    inline void Reset()
    {
        for (auto& dict : m_dicts)
        {
            dict.Reset();
        }
    }
    inline void Clear()
    {
        for (auto& dict : m_dicts)
        {
            dict.Clear();
        }
    }
    inline i32 Find(K key) const
    {
        const u32 hash = GetHash(key);
        return m_dicts[GetSlot(hash)].Find(key, hash);
    }
    inline bool Contains(K key) const { return Find(key) != -1; }
    inline const V* Get(K key) const
    {
        const u32 hash = GetHash(key);
        return m_dicts[GetSlot(hash)].Get(key, hash);
    }
    inline const V GetOrNull(K key) const
    {
        const V* ptr = Get(key);
        return ptr ? *ptr : nullptr;
    }
    inline V* Get(K key)
    {
        const u32 hash = GetHash(key);
        return m_dicts[GetSlot(hash)].Get(key, hash);
    }
    inline V GetOrNull(K key)
    {
        V* ptr = Get(key);
        return ptr ? *ptr : nullptr;
    }
    inline void Remove(K key)
    {
        const u32 hash = GetHash(key);
        m_dicts[GetSlot(hash)].Remove(key);
    }
    inline V& operator[](K key)
    {
        const u32 hash = GetHash(key);
        Dict<K, V>& dict = m_dicts[GetSlot(hash)];
        const i32 i = dict.Find(key, hash);
        if (i != -1)
        {
            return dict.m_values[i];
        }
        dict.m_hashes.Grow() = hash;
        dict.m_keys.Grow() = key;
        return dict.m_values.Grow();
    }
};
