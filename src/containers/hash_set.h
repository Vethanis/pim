#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include <string.h>

template<
    typename K,
    const Comparator<K>& cmp>
    struct HashSet
{
    u32 m_allocator : 4;
    u32 m_count : 28;
    u32 m_width;
    u32* m_hashes;
    K* m_keys;

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
        m_hashes = 0;
        m_keys = 0;
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
        }
    }

    static HashSet Build(AllocType allocator, Slice<const K> keys)
    {
        HashSet set;
        set.Init(allocator);
        set.Reserve(keys.size());
        for (const K& key : keys)
        {
            set.Add(key);
        }
        return set;
    }

    // ------------------------------------------------------------------------

    bool Contains(K key) const
    {
        return HashUtil::Contains<K>(
            cmp, m_width,
            m_hashes, m_keys,
            key);
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

        u32* newHashes = Allocator::CallocT<u32>(GetAllocator(), newWidth);
        K* newKeys = Allocator::CallocT<K>(GetAllocator(), newWidth);

        const u32* oldHashes = m_hashes;
        const K* oldKeys = m_keys;

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
        }

        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);

        m_width = newWidth;
        m_hashes = newHashes;
        m_keys = newKeys;
    }

    void Reserve(u32 cap)
    {
        if ((cap * 10u) >= (m_width * 7u))
        {
            Resize(Max(cap, 8u));
        }
    }

    void Trim()
    {
        Resize(m_count);
    }

    bool Add(K key)
    {
        Reserve(m_count + 1);
        i32 i = HashUtil::Insert<K>(
            cmp, m_width,
            m_hashes, m_keys,
            key);
        if (i != -1)
        {
            ++m_count;
            return true;
        }
        return false;
    }

    bool Remove(K key)
    {
        i32 i = HashUtil::Remove<K>(
            cmp, m_width,
            m_hashes, m_keys,
            key);
        if (i != -1)
        {
            --m_count;
            return true;
        }
        return false;
    }

    // ------------------------------------------------------------------------

    struct iterator
    {
        u32 m_i;
        const u32 m_width;
        const u32* const m_hashes;
        K* const m_keys;

        inline iterator(HashSet& set)
            : m_i(0),
            m_width(set.m_width),
            m_hashes(set.m_hashes),
            m_keys(set.m_keys)
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

        inline K& operator*() { return m_keys[m_i]; }
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

        inline const_iterator(const HashSet& set)
            : m_i(0),
            m_width(set.m_width),
            m_hashes(set.m_hashes),
            m_keys(set.m_keys)
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
        }

        inline bool operator!=(const const_iterator& rhs) const
        {
            return m_i < m_width;
        }

        inline const_iterator& operator++()
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
            return *this;
        }

        inline const K& operator*() const { return m_keys[m_i]; }
    };

    inline const_iterator begin() const { return const_iterator(*this); }
    inline const_iterator end() const { return const_iterator(*this); }

    // ------------------------------------------------------------------------
};
