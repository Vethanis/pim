#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include "containers/array.h"
#include <string.h>

template<typename K>
struct HashSet
{
private:
    u32* m_hashes;
    K* m_keys;
    u32 m_count;
    u32 m_width;
    AllocType m_allocator;

public:
    u32 size() const { return m_count; }
    u32 capacity() const { return m_width; }
    AllocType GetAllocator() const { return m_allocator; }

    void Init(AllocType allocator)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
    }

    void Init(AllocType allocator, u32 minCap)
    {
        Init(allocator);
        Reserve(minCap);
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

    void Reserve(u32 minCount)
    {
        const u32 newWidth = HashUtil::ToPow2(minCount);
        const u32 oldWidth = m_width;
        if (newWidth <= oldWidth)
        {
            return;
        }

        const AllocType allocator = m_allocator;
        u32* newHashes = Allocator::CallocT<u32>(allocator, newWidth);
        K* newKeys = Allocator::CallocT<K>(allocator, newWidth);

        u32* oldHashes = m_hashes;
        K* oldKeys = m_keys;
        const u32 oldMask = oldWidth - 1u;
        const u32 newMask = newWidth - 1u;

        for (u32 i = 0u; i < oldWidth; ++i)
        {
            const u32 hash = oldHashes[i];
            if (HashUtil::IsValidHash(hash))
            {
                u32 j = hash;
                while (true)
                {
                    j &= newMask;
                    if (!newHashes[j])
                    {
                        newHashes[j] = hash;
                        newKeys[j] = oldKeys[i];
                        break;
                    }
                    ++j;
                }
            }
        }

        m_hashes = newHashes;
        m_keys = newKeys;
        m_width = newWidth;

        Allocator::Free(oldHashes);
        Allocator::Free(oldKeys);
    }

    i32 Find(u32 keyHash, K key) const
    {
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;

        u32 width = m_width;
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            const u32 jHash = hashes[j];
            if (HashUtil::IsEmpty(jHash))
            {
                break;
            }
            if (jHash == keyHash)
            {
                if (key == keys[j])
                {
                    return (i32)j;
                }
            }
        }

        return -1;
    }

    i32 Find(K key) const
    {
        return Find(HashUtil::Hash(key), key);
    }

    bool Contains(K key) const
    {
        return Find(key) != -1;
    }

    bool Add(K key)
    {
        const u32 keyHash = HashUtil::Hash(key);
        if (Find(keyHash, key) != -1)
        {
            return false;
        }

        Reserve(size() + 3u);

        u32* const hashes = m_hashes;
        K* const keys = m_keys;
        const u32 mask = m_width - 1u;

        u32 j = keyHash;
        while (true)
        {
            j &= mask;
            if (HashUtil::IsEmptyOrTomb(hashes[j]))
            {
                hashes[j] = keyHash;
                keys[j] = key;
                ++m_count;
                return true;
            }
            ++j;
        }

        return false;
    }

    bool Remove(K key)
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            m_hashes[i] = HashUtil::TombMask;
            memset(m_keys + i, 0, sizeof(K));
            --m_count;
            return true;
        }
        return false;
    }

    bool Get(K key, K& keyOut) const
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            keyOut = m_keys[i];
            return true;
        }
        return false;
    }

    struct const_iterator
    {
        const HashSet& m_set;
        u32 m_index;

        const_iterator(const HashSet& set, u32 index) :
            m_set(set), m_index(index)
        {
            const u32 width = m_set.capacity();
            const u32* const hashes = m_set.m_hashes;
            u32 i = m_index;
            for (; i < width; ++i)
            {
                if (HashUtil::IsValidHash(hashes[i]))
                {
                    break;
                }
            }
            m_index = i;
        }

        const_iterator& operator++()
        {
            const u32 width = m_set.capacity();
            const u32* const hashes = m_set.m_hashes;
            u32 i = ++m_index;
            for (; i < width; ++i)
            {
                if (HashUtil::IsValidHash(hashes[i]))
                {
                    break;
                }
            }
            m_index = i;
            return *this;
        }

        bool operator!=(const_iterator rhs) const
        {
            return m_index != rhs.m_index;
        }

        K operator*() const
        {
            const i32 i = m_index;
            return m_set.m_keys[i];
        }
    };

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, capacity()); }
};

template<typename K>
static HashSet<K> CreateHashSet(AllocType allocator, u32 minCap)
{
    HashSet<K> set = {};
    set.Init(allocator, minCap);
    return set;
}
