#pragma once

#include "common/macro.h"
#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "containers/hash.hpp"
#include <new>

template<typename K, typename V>
class Dict final
{
private:
    i32 m_width;
    i32 m_count;
    u32* pim_noalias m_hashes;
    K* pim_noalias m_keys;
    V* pim_noalias m_values;
    EAlloc m_allocator;

public:
    i32 Size() const { return m_count; }
    i32 Capacity() const { return m_width; }

    explicit Dict(EAlloc allocator = EAlloc_Perm)
    {
        m_hashes = NULL;
        m_keys = NULL;
        m_values = NULL;
        m_width = 0;
        m_count = 0;
        m_allocator = allocator;
    }

    ~Dict()
    {
        Reset();
    }

    explicit Dict(const Dict& other)
    {
        m_hashes = NULL;
        m_keys = NULL;
        m_values = NULL;
        m_width = 0;
        m_count = 0;
        m_allocator = other.m_allocator;
        const u32 width = other.m_width;
        u32 const *const pim_noalias otherHashes = other.m_hashes;
        K const *const pim_noalias otherKeys = other.m_keys;
        V const *const pim_noalias otherValues = other.m_values;
        Reserve(other.m_count);
        for (u32 i = 0; i < width; ++i)
        {
            if (hashutil_valid(otherHashes[i]))
            {
                AddCopy(otherKeys[i], otherValues[i]);
            }
        }
    }
    Dict& operator=(const Dict& other)
    {
        Reset();

        const u32 width = other.m_width;
        u32 const *const pim_noalias otherHashes = other.m_hashes;
        K const *const pim_noalias otherKeys = other.m_keys;
        V const *const pim_noalias otherValues = other.m_values;
        Reserve(other.m_count);
        for (u32 i = 0; i < width; ++i)
        {
            if (hashutil_valid(otherHashes[i]))
            {
                AddCopy(otherKeys[i], otherValues[i]);
            }
        }
        return *this;
    }

    explicit Dict(Dict&& other)
    {
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
        other.m_allocator = m_allocator;
    }
    Dict& operator=(Dict&& other)
    {
        Reset();
        memcpy(this, &other, sizeof(*this));
        memset(&other, 0, sizeof(*this));
        other.m_allocator = m_allocator;
        return *this;
    }

    void Clear()
    {
        u32 *const pim_noalias hashes = m_hashes;
        K *const pim_noalias keys = m_keys;
        V *const pim_noalias values = m_values;
        const u32 width = m_width;
        for (u32 i = 0; i < width; ++i)
        {
            if (hashutil_valid(hashes[i]))
            {
                hashes[i] = 0;
                keys[i].~K();
                values[i].~V();
            }
        }
        m_count = 0;
    }

    void Reset()
    {
        Clear();
        pim_free(m_hashes);
        pim_free(m_keys);
        pim_free(m_values);
        m_keys = NULL;
        m_hashes = NULL;
        m_values = NULL;
        m_width = 0;
    }

    void Reserve(i32 count)
    {
        ASSERT(count >= 0);
        count = count > 16 ? count : 16;
        const u32 newWidth = NextPow2((u32)count) * 2;
        const u32 oldWidth = m_width;
        if (newWidth <= oldWidth)
        {
            return;
        }

        u32 *const pim_noalias oldHashes = m_hashes;
        K *const pim_noalias oldKeys = m_keys;
        V *const pim_noalias oldValues = m_values;

        const EAlloc allocator = m_allocator;
        u32 *const pim_noalias newHashes = (u32*)pim_calloc(allocator, sizeof(newHashes[0]) * newWidth);
        K *const pim_noalias newKeys = (K*)pim_calloc(allocator, sizeof(newKeys[0]) * newWidth);
        V *const pim_noalias newValues = (V*)pim_calloc(allocator, sizeof(newValues[0]) * newWidth);

        const u32 newMask = newWidth - 1;
        for (u32 i = 0; i < oldWidth;)
        {
            const u32 hash = oldHashes[i];
            if (hashutil_valid(hash))
            {
                u32 j = hash;
                while (true)
                {
                    j &= newMask;
                    if (!newHashes[j])
                    {
                        newHashes[j] = hash;
                        memcpy(newKeys + j, oldKeys + i, sizeof(K));
                        memcpy(newValues + j, oldValues + i, sizeof(V));
                        goto next;
                    }
                    ++j;
                }
            }
        next:
            ++i;
        }

        pim_free(oldHashes);
        pim_free(oldKeys);
        pim_free(oldValues);

        m_width = newWidth;
        m_hashes = newHashes;
        m_keys = newKeys;
        m_values = newValues;
    }

    bool Contains(const K& pim_noalias key) const
    {
        return Find(key) >= 0;
    }

    V* Get(const K& pim_noalias key)
    {
        i32 i = Find(key);
        if (i >= 0)
        {
            return m_values + i;
        }
        return NULL;
    }

    bool SetMove(const K& pim_noalias key, V&& pim_noalias value)
    {
        i32 i = Find(key);
        if (i >= 0)
        {
            m_values[i] = (V&& pim_noalias)value;
            return true;
        }
        return false;
    }
    bool SetCopy(const K& pim_noalias key, const V& pim_noalias value)
    {
        i32 i = Find(key);
        if (i >= 0)
        {
            m_values[i] = value;
            return true;
        }
        return false;
    }

    bool AddMove(const K& pim_noalias key, V&& pim_noalias value)
    {
        if (Contains(key))
        {
            return false;
        }
        Reserve(++m_count);
        u32 keyHash;
        i32 i = FindInsertion(key, keyHash);
        m_hashes[i] = keyHash;
        new (m_keys + i) K(key);
        new (m_values + i) V((V&&)value);
        return true;
    }
    bool AddCopy(const K& pim_noalias key, const V& pim_noalias value)
    {
        if (Contains(key))
        {
            return false;
        }
        Reserve(++m_count);
        u32 keyHash;
        i32 i = FindInsertion(key, keyHash);
        m_hashes[i] = keyHash;
        new (m_keys + i) K(key);
        new (m_values + i) V(value);
        return true;
    }

    bool Remove(const K& pim_noalias key, V* pim_noalias valueOut = NULL)
    {
        i32 i = Find(key);
        if (i < 0)
        {
            return false;
        }

        if (valueOut)
        {
            *valueOut = (V&& pim_noalias)m_values[i];
        }

        m_hashes[i] |= hashutil_tomb_mask;
        m_keys[i].~K();
        m_values[i].~V();

        --m_count;
        return true;
    }

    V& pim_noalias operator[](const K& pim_noalias key)
    {
        i32 i = Find(key);
        if (i >= 0)
        {
            return m_values[i];
        }

        Reserve(++m_count);

        u32 keyHash;
        i = FindInsertion(key, keyHash);
        m_hashes[i] = keyHash;
        new (m_keys + i) K(key);
        new (m_values + i) V();
        return m_values[i];
    }

    // ------------------------------------------------------------------------

    struct Pair final
    {
        const K& pim_noalias key;
        V& pim_noalias value;
    };
    class iterator final
    {
    private:
        i32 m_index;
        i32 m_width;
        u32 const *const pim_noalias m_hashes;
        K const *const pim_noalias m_keys;
        V *const pim_noalias m_values;
    public:
        explicit iterator(Dict& dict, i32 index) :
            m_index(index),
            m_width(dict.m_width),
            m_hashes(dict.m_hashes),
            m_keys(dict.m_keys),
            m_values(dict.m_values)
        {
            m_index = Iterate(m_hashes, m_width, m_index);
        }
        bool operator!=(const iterator&) const { return m_index < m_width; }
        iterator& operator++()
        {
            m_index = Iterate(m_hashes, m_width, m_index + 1);
            return *this;
        }
        Pair operator*()
        {
            i32 i = m_index;
            ASSERT(i < m_width);
            ASSERT(hashutil_valid(m_hashes[i]));
            return Pair{ m_keys[i], m_values[i] };
        }
    };
    iterator begin() { return iterator(*this, 0); }
    iterator end() { return iterator(*this, m_width); }

    // ------------------------------------------------------------------------

    struct ConstPair final
    {
        const K& pim_noalias key;
        const V& pim_noalias value;
    };
    class const_iterator final
    {
    private:
        i32 m_index;
        i32 m_width;
        u32 const *const pim_noalias m_hashes;
        K const *const pim_noalias m_keys;
        V const *const pim_noalias m_values;
    public:
        explicit const_iterator(const Dict& dict, i32 index) :
            m_index(index),
            m_width(dict.m_width),
            m_hashes(dict.m_hashes),
            m_keys(dict.m_keys),
            m_values(dict.m_values)
        {
            m_index = Iterate(m_hashes, m_width, m_index);
        }
        bool operator!=(const const_iterator&) const { return m_index < m_width; }
        const_iterator& operator++()
        {
            m_index = Iterate(m_hashes, m_width, m_index + 1);
            return *this;
        }
        ConstPair operator*()
        {
            i32 i = m_index;
            ASSERT(i < m_width);
            ASSERT(hashutil_valid(m_hashes[i]));
            return ConstPair{ m_keys[i], m_values[i] };
        }
    };
    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, m_width); }

    // ------------------------------------------------------------------------

private:

    static i32 Iterate(u32 const *const pim_noalias hashes, i32 width, i32 index)
    {
        while (index < width)
        {
            if (hashutil_valid(hashes[index]))
            {
                break;
            }
            ++index;
        }
        return index;
    }

    i32 Find(const K& pim_noalias key) const
    {
        const u32 keyHash = Hashable<K>::HashOf(key);

        u32 width = m_width;
        const u32 mask = width - 1;
        u32 const *const pim_noalias hashes = m_hashes;
        K const *const pim_noalias keys = m_keys;

        u32 j = keyHash;
        while (width--)
        {
            j &= mask;
            const u32 jHash = hashes[j];
            if (hashutil_empty(jHash))
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
            ++j;
        }

        return -1;
    }

    i32 FindInsertion(const K& pim_noalias key, u32& pim_noalias hashOut) const
    {
        const u32 mask = m_width - 1;
        const u32 keyHash = Hashable<K>::HashOf(key);
        hashOut = keyHash;
        u32 const *const pim_noalias hashes = m_hashes;
        u32 j = keyHash;
        while (true)
        {
            j &= mask;
            if (hashutil_empty_or_tomb(hashes[j]))
            {
                return (i32)j;
            }
            ++j;
        }
    }
};

