#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include "containers/array.h"
#include <string.h>

template<typename K, typename V>
struct HashDict
{
private:
    u32 m_count;
    u32 m_width;
    u32* m_hashes;
    K* m_keys;
    V* m_values;
    EAlloc m_allocator;

public:
    EAlloc GetAllocator() const { return m_allocator; }
    u32 size() const { return m_count; }
    u32 capacity() const { return m_width; }

    void Init(EAlloc allocator = EAlloc_Perm)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
    }

    void Reset()
    {
        pim_free(m_hashes);
        pim_free(m_keys);
        pim_free(m_values);
        m_hashes = 0;
        m_values = 0;
        m_width = 0;
        m_count = 0;
    }

    void Clear()
    {
        m_count = 0;
        u32* hashes = m_hashes;
        K* keys = m_keys;
        V* values = m_values;
        const u32 width = m_width;
        if (hashes)
        {
            memset(hashes, 0, sizeof(u32) * width);
            memset(keys, 0, sizeof(K) * width);
            memset(values, 0, sizeof(V) * width);
        }
    }

    static HashDict Build(EAlloc allocator, Slice<const K> keys, Slice<const V> values)
    {
        HashDict dict;
        dict.Init(allocator);
        const i32 len = keys.size();
        dict.Reserve(len);
        for (i32 i = 0; i < len; ++i)
        {
            dict.Add(keys[i], values[i]);
        }
        return dict;
    }

    // ------------------------------------------------------------------------

    void Reserve(u32 minCount)
    {
        minCount = Max(minCount, 16u);
        const u32 newWidth = HashUtil::ToPow2(minCount);
        const u32 oldWidth = capacity();
        if (newWidth <= oldWidth)
        {
            return;
        }

        const EAlloc allocator = GetAllocator();
        u32* newHashes = pim_tcalloc(u32, allocator, newWidth);
        K* newKeys = pim_tcalloc(K, allocator, newWidth);
        V* newValues = pim_tmalloc(V, allocator, newWidth);

        u32* oldHashes = m_hashes;
        K* oldKeys = m_keys;
        V* oldValues = m_values;

        const u32 newMask = newWidth - 1u;
        for (u32 i = 0u; i < oldWidth;)
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
                        newValues[j] = oldValues[i];
                        goto next;
                    }
                    ++j;
                }
            }
        next:
            ++i;
        }

        m_hashes = newHashes;
        m_keys = newKeys;
        m_values = newValues;
        m_width = newWidth;

        pim_free(oldHashes);
        pim_free(oldKeys);
        pim_free(oldValues);
    }

    i32 Find(u32 keyHash, K key) const
    {
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;

        u32 width = m_width;
        const u32 mask = width - 1u;

        for (u32 j = keyHash; width--; ++j)
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

    bool Add(K key, const V& value)
    {
        const u32 keyHash = HashUtil::Hash(key);
        if (Find(keyHash, key) != -1)
        {
            return false;
        }

        Reserve(size() + 3u);

        u32* const hashes = m_hashes;
        K* const keys = m_keys;
        V* const values = m_values;
        const u32 mask = m_width - 1u;

        u32 j = keyHash;
        while(true)
        {
            j &= mask;
            if (HashUtil::IsEmptyOrTomb(hashes[j]))
            {
                hashes[j] = keyHash;
                keys[j] = key;
                values[j] = value;
                ++m_count;
                return true;
            }
            ++j;
        }
    }

    bool Remove(K key, V& valueOut)
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            m_hashes[i] = HashUtil::TombMask;
            valueOut = m_values[i];
            memset(m_keys + i, 0, sizeof(K));
            memset(m_values + i, 0, sizeof(V));
            --m_count;
            return true;
        }
        return false;
    }

    bool Get(K key, V& valueOut) const
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            valueOut = m_values[i];
            return true;
        }
        return false;
    }

    bool Set(K key, const V& valueIn)
    {
        const i32 i = Find(key);
        if (i != -1)
        {
            m_values[i] = valueIn;
            return true;
        }
        return false;
    }

    struct Pair
    {
        K key;
        V value;
    };

    struct const_iterator
    {
        const HashDict<K, V>& m_dict;
        u32 m_index;

        const_iterator(const HashDict<K, V>& dict, u32 index) : m_dict(dict), m_index(index)
        {
            const u32* const hashes = m_dict.m_hashes;
            const u32 width = m_dict.capacity();
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

        Pair operator*() const
        {
            const u32 i = m_index;
            ASSERT(i < m_dict.capacity());
            return { m_dict.m_keys[i], m_dict.m_values[i] };
        }

        bool operator!=(const_iterator rhs) const
        {
            return m_index != rhs.m_index;
        }

        const_iterator& operator++()
        {
            const u32* const hashes = m_dict.m_hashes;
            const u32 width = m_dict.capacity();
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
    };

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, capacity()); }
};
