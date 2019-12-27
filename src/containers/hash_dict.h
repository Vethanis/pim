#pragma once

#include "containers/hash_util.h"

template<
    typename K,
    typename V,
    const Comparator<K>& cmp>
    struct HashDict
{
    u32 m_allocator : 4;
    u32 m_count : 28;
    u32 m_width;
    u32* m_hashes;
    K* m_keys;
    V* m_values;

    // ------------------------------------------------------------------------

    inline i32 Size() const { return (i32)m_count; }
    inline i32 Capacity() const { return (i32)m_width; }
    inline AllocType GetAllocType() const { return (AllocType)m_allocator; }

    void Init(AllocType allocator)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
    }

    void Reset()
    {
        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);
        Allocator::Free(m_values);
        m_hashes = 0;
        m_values = 0;
        m_width = 0;
        m_count = 0;
    }

    void Clear()
    {
        m_count = 0;
        if (m_hashes)
        {
            memset(m_hashes, 0, sizeof(u32) * m_width);
            memset(m_keys, 0, sizeof(K) * m_width);
            memset(m_values, 0, sizeof(V) * m_width);
        }
    }

    // ------------------------------------------------------------------------

    inline bool Contains(K key) const
    {
        return HashUtil::Contains<K>(
            cmp, m_width, m_hashes, m_keys, key);
    }

    void Resize(u32 width)
    {
        if (width == m_width)
        {
            return;
        }
        if (width == 0)
        {
            Reset();
            return;
        }
        ASSERT(width >= m_count);
        ASSERT(HashUtil::IsPow2(width));

        u32* newHashes = Allocator::CallocT<u32>(GetAllocType(), width);
        K* newKeys = Allocator::CallocT<K>(GetAllocType(), width);
        V* newValues = Allocator::AllocT<V>(GetAllocType(), width);

        const u32* oldHashes = m_hashes;
        const K* oldKeys = m_keys;
        const V* oldValues = m_values;
        const u32 oldWidth = m_width;

        for (u32 i = 0u; i < oldWidth; ++i)
        {
            const u32 hash = oldHashes[i];
            if (HashUtil::IsEmptyOrTomb(hash))
            {
                continue;
            }
            i32 j = HashUtil::Insert<K>(
                cmp, width,
                newHashes, newKeys,
                hash, oldKeys[i]);
            ASSERT(j != -1);
            newValues[j] = oldValues[i];
        }

        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);
        Allocator::Free(m_values);

        m_width = width;
        m_hashes = newHashes;
        m_keys = newKeys;
        m_values = newValues;
    }

    void Trim()
    {
        Resize(HashUtil::TrimWidth(m_width, m_count));
    }

    inline bool Add(K key, V value)
    {
        Resize(HashUtil::GrowWidth(m_width, m_count));
        i32 i = HashUtil::Insert<K>(
            cmp, m_width, m_hashes, m_keys, key);
        if (i != -1)
        {
            m_values[i] = value;
            ++m_count;
            return true;
        }
        return false;
    }

    inline bool Remove(K key)
    {
        i32 i = HashUtil::Remove<K>(
            cmp, m_width, m_hashes, m_keys, key);
        if (i != -1)
        {
            --m_count;
            return true;
        }
        return false;
    }

    inline const V* Get(K key) const
    {
        i32 i = HashUtil::Find<K>(
            cmp, m_width, m_hashes, m_keys, key);
        return (i == -1) ? nullptr : m_values + i;
    }

    inline V* Get(K key)
    {
        i32 i = HashUtil::Find<K>(
            cmp, m_width, m_hashes, m_keys, key);
        return (i == -1) ? nullptr : m_values + i;
    }

    inline bool Set(K key, V value)
    {
        i32 i = HashUtil::Find<K>(
            cmp, m_width, m_hashes, m_keys, key);
        if (i == -1)
        {
            return false;
        }
        m_values[i] = value;
        return true;
    }

    inline V& operator[](K key)
    {
        const u32 hash = HashUtil::Hash<K>(cmp, key);
        i32 i = HashUtil::Find<K>(
            cmp, m_width, m_hashes, m_keys, hash, key);
        if (i == -1)
        {
            Resize(HashUtil::GrowWidth(m_width, m_count));
            i = HashUtil::Insert<K>(
                cmp, m_width, m_hashes, m_keys, hash, key);
            ASSERT(i != -1);
            memset(m_values + i, 0, sizeof(V));
            ++m_count;
        }
        return m_values[i];
    }

    struct Pair
    {
        K& Key;
        V& Value;
    };

    struct iterator
    {
        u32 m_i;
        const u32 m_width;
        const u32* const m_hashes;
        K* const m_keys;
        V* const m_values;

        inline iterator(HashDict& dict, bool isBegin)
        {
            m_width = dict.m_width;
            m_hashes = dict.m_hashes;
            m_keys = dict.m_keys;
            m_values = dict.m_values;
            m_i = isBegin ? 0 : m_width;
        }

        inline bool operator!=(const iterator& rhs) const
        {
            return m_i != rhs.m_i;
        }

        inline iterator& operator++()
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
            return *this;
        }

        inline Pair& operator*()
        {
            return Pair{ m_keys[m_i], m_values[m_i] };
        }
    };

    inline iterator begin()
    {
        return iterator(*this, true);
    }
    inline iterator end()
    {
        return iterator(*this, false);
    }
};
