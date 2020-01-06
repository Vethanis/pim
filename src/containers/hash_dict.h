#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include <string.h>

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

    inline i32 size() const { return (i32)m_count; }
    inline i32 capacity() const { return (i32)m_width; }
    inline AllocType GetAllocator() const { return (AllocType)m_allocator; }

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

    static HashDict Build(AllocType allocator, Slice<const K> keys, Slice<const V> values)
    {
        const i32 count = keys.size();
        ASSERT(count == values.size());
        HashDict dict;
        dict.Init(allocator);
        dict.Reserve(count);
        for (i32 i = 0; i < count; ++i)
        {
            dict.Add(keys[i], values[i]);
        }
        return dict;
    }

    // ------------------------------------------------------------------------

    inline bool Contains(K key) const
    {
        return HashUtil::Contains<K>(
            cmp, m_width, m_hashes, m_keys, key);
    }

    void Resize(u32 minCount)
    {
        ASSERT(minCount >= m_count);
        const u32 newWidth = HashUtil::ToPow2(minCount);
        const u32 oldWidth = m_width;
        if (newWidth == oldWidth)
        {
            return;
        }
        if (newWidth == 0)
        {
            Reset();
            return;
        }

        const AllocType allocator = GetAllocator();
        u32* newHashes = Allocator::CallocT<u32>(allocator, newWidth);
        K* newKeys = Allocator::CallocT<K>(allocator, newWidth);
        V* newValues = Allocator::AllocT<V>(allocator, newWidth);

        const u32* oldHashes = m_hashes;
        const K* oldKeys = m_keys;
        const V* oldValues = m_values;

        for (u32 i = 0u; i < oldWidth; ++i)
        {
            const u32 hash = oldHashes[i];
            if (HashUtil::IsEmptyOrTomb(hash))
            {
                continue;
            }
            i32 j = HashUtil::Insert<K>(
                cmp, newWidth,
                newHashes, newKeys,
                hash, oldKeys[i]);
            ASSERT(j != -1);
            newValues[j] = oldValues[i];
        }

        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);
        Allocator::Free(m_values);

        m_width = newWidth;
        m_hashes = newHashes;
        m_keys = newKeys;
        m_values = newValues;
    }

    void Reserve(u32 cap)
    {
        if ((cap * 10u) >= (m_width * 7u))
        {
            Resize(cap > 8u ? cap : 8u);
        }
    }

    void Trim()
    {
        Resize(m_count);
    }

    inline bool Add(K key, V value)
    {
        Reserve(m_count + 1u);
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
            Reserve(m_count + 1u);
            i = HashUtil::Insert<K>(
                cmp, m_width, m_hashes, m_keys, hash, key);
            ASSERT(i != -1);
            memset(m_values + i, 0, sizeof(V));
            ++m_count;
        }
        return m_values[i];
    }

    // ------------------------------------------------------------------------

    struct iterator
    {
        u32 m_i;
        const u32 m_width;
        const u32* const m_hashes;
        K* const m_keys;
        V* const m_values;

        inline iterator(HashDict& dict)
            : m_i(0),
            m_width(dict.m_width),
            m_hashes(dict.m_hashes),
            m_keys(dict.m_keys),
            m_values(dict.m_values)
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
        }

        inline bool operator!=(const iterator& rhs) const
        {
            return m_i < m_width;
        }

        inline iterator& operator++()
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
            return *this;
        }

        struct Pair
        {
            K& Key;
            V& Value;
        };

        inline Pair operator*()
        {
            return Pair{ m_keys[m_i], m_values[m_i] };
        }
    };

    inline iterator begin() { return iterator(*this); }
    inline iterator end() { return iterator(*this); }

    // ------------------------------------------------------------------------

    struct const_iterator
    {
        u32 m_i;
        const u32 m_width;
        const u32* const m_hashes;
        const K* const m_keys;
        const V* const m_values;

        inline const_iterator(const HashDict& dict)
            : m_i(0),
            m_width(dict.m_width),
            m_hashes(dict.m_hashes),
            m_keys(dict.m_keys),
            m_values(dict.m_values)
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
        }

        inline bool operator!=(const const_iterator&) const
        {
            return m_i < m_width;
        }

        inline const_iterator& operator++()
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
            return *this;
        }

        struct Pair
        {
            const K& Key;
            const V& Value;
        };

        inline Pair operator*() const
        {
            return Pair{ m_keys[m_i], m_values[m_i] };
        }
    };

    inline const_iterator begin() const { return const_iterator(*this); }
    inline const_iterator end() const { return const_iterator(*this); }

    // ------------------------------------------------------------------------
};
