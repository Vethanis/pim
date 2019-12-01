#pragma once

#include "containers/array.h"

template<typename K, typename V>
struct Dict
{
    Array<u8> m_hashes;
    Array<K> m_keys;
    Array<V> m_values;

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
    inline static u8 ToHash(K key)
    {
        return 0xff & Fnv32T(key);
    }
    inline i32 Find(K key) const
    {
        const u8 hash = ToHash(key);
        const i32 count = m_hashes.Size();
        const u8* const hashes = m_hashes.begin();
        const K* const keys = m_keys.begin();
        for (i32 i = count - 1; i >= 0; --i)
        {
            if (hashes[i] == hash)
            {
                if (keys[i] == key)
                {
                    return i;
                }
            }
        }
        return -1;
    }
    inline bool Contains(K key) const { return Find(key) != -1; }
    inline const V* Get(K key) const
    {
        const i32 i = Find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline V* Get(K key)
    {
        const i32 i = Find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline void Remove(K key)
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            m_hashes.Remove(i);
            m_keys.Remove(i);
            m_values.Remove(i);
        }
    }
    inline V& operator[](K key)
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            return m_values[i];
        }
        else
        {
            m_hashes.Grow() = ToHash(key);
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

    StaticAssert((t_width & (t_width - 1u)) == 0);

    inline static u32 GetSlot(K key)
    {
        return Fnv32T(key) & Mask;
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
        return m_dicts[GetSlot(key)].Find(key);
    }
    inline bool Contains(K key) const { return Find(key) != -1; }
    inline const V* Get(K key) const
    {
        return m_dicts[GetSlot(key)].Get(key);
    }
    inline V* Get(K key)
    {
        return m_dicts[GetSlot(key)].Get(key);
    }
    inline void Remove(K key)
    {
        m_dicts[GetSlot(key)].Remove(key);
    }
    inline V& operator[](K key)
    {
        return m_dicts[GetSlot(key)][key];
    }
};
