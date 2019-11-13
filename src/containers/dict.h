#pragma once

#include "containers/array.h"

template<typename K, typename V>
struct Dict
{
    Array<u8> m_hashes;
    Array<K> m_keys;
    Array<V> m_values;

    inline i32 size() const { return m_keys.size(); }
    inline i32 capacity() const { return m_keys.capacity(); }
    inline bool empty() const { return m_keys.empty(); }
    inline bool full() const { return m_keys.full(); }

    inline void init()
    {
        m_hashes.init();
        m_keys.init();
        m_values.init();
    }
    inline void reset()
    {
        m_hashes.reset();
        m_keys.reset();
        m_values.reset();
    }
    inline void clear()
    {
        m_hashes.clear();
        m_keys.clear();
        m_values.clear();
    }
    inline static u8 to_hash(K key)
    {
        return 0xff & Fnv32T(key);
    }
    inline i32 find(K key) const
    {
        const u8 hash = to_hash(key);
        const i32 count = m_hashes.size();
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
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline V* get(K key)
    {
        const i32 i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline void remove(K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            m_hashes.remove(i);
            m_keys.remove(i);
            m_values.remove(i);
        }
    }
    inline V& operator[](K key)
    {
        const i32 i = find(key);
        if (i != -1)
        {
            return m_values[i];
        }
        else
        {
            m_hashes.grow() = to_hash(key);
            m_keys.grow() = key;
            return m_values.grow();
        }
    }
};

template<u32 t_width, typename K, typename V>
struct DictTable
{
    static constexpr u32 Width = t_width;
    static constexpr u32 Mask = t_width - 1u;

    inline static u32 GetSlot(K key)
    {
        return Fnv32T(key) & Mask;
    }

    Dict<K, V> m_dicts[Width];

    inline void init()
    {
        for (auto& dict : m_dicts)
        {
            dict.init();
        }
    }
    inline void reset()
    {
        for (auto& dict : m_dicts)
        {
            dict.reset();
        }
    }
    inline void clear()
    {
        for (auto& dict : m_dicts)
        {
            dict.clear();
        }
    }
    inline i32 find(K key) const
    {
        return m_dicts[GetSlot(key)].find(key);
    }
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        return m_dicts[GetSlot(key)].get(key);
    }
    inline V* get(K key)
    {
        return m_dicts[GetSlot(key)].get(key);
    }
    inline void remove(K key)
    {
        m_dicts[GetSlot(key)].remove(key);
    }
    inline V& operator[](K key)
    {
        return m_dicts[GetSlot(key)][key];
    }
};
