#pragma once

#include "containers/hash_util.h"

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

    // ------------------------------------------------------------------------

    bool Contains(K key) const
    {
        return HashUtil::Contains<K>(
            cmp, m_width,
            m_hashes, m_keys,
            key);
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

        const u32* oldHashes = m_hashes;
        const K* oldKeys = m_keys;
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
        }

        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);

        m_width = width;
        m_hashes = newHashes;
        m_keys = newKeys;
    }

    void Trim()
    {
        Resize(HashUtil::TrimWidth(m_width, m_count));
    }

    bool Add(K key)
    {
        Resize(HashUtil::GrowWidth(m_width, m_count));
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

    struct iterator
    {
        u32 m_i;
        const u32 m_width;
        const u32* const m_hashes;
        K* const m_keys;

        inline iterator(HashSet& set, bool isBegin)
            :
            m_i(isBegin ? 0, set.m_width),
            m_width(set.m_width),
            m_hashes(set.m_hashes),
            m_keys(set.m_keys)
        {}

        inline bool operator!=(const iterator& rhs) const
        {
            return m_i != rhs.m_i;
        }

        inline iterator& operator++()
        {
            m_i = HashUtil::Iterate(m_hashes, m_width, m_i);
            return *this;
        }

        inline K& operator*()
        {
            return m_keys[m_i];
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
