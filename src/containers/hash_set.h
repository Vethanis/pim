#pragma once

#include "common/macro.h"
#include "common/comparator.h"
#include "allocator/allocator.h"
#include <string.h>

namespace HashUtil
{
    static constexpr u32 EmptyHash = 0x0u;
    static constexpr u32 TombHash = 0xffffffffu;

    // ------------------------------------------------------------------------

    static constexpr bool IsPow2(u32 x)
    {
        return (x & (x - 1u)) == 0u;
    }

    static constexpr bool IsEmpty(u32 hash)
    {
        return hash == EmptyHash;
    }

    static constexpr bool IsTomb(u32 hash)
    {
        return hash == TombHash;
    }

    static constexpr bool IsEmptyOrTomb(u32 hash)
    {
        return IsEmpty(hash) || IsTomb(hash);
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static u32 Hash(const Comparator<K> cmp, K key)
    {
        u32 hash = cmp.Hash(key);
        hash = IsEmpty(hash) ? (hash + 1u) : hash;
        hash = IsTomb(hash) ? (hash - 1u) : hash;
        return hash;
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static i32 Find(
        const Comparator<K> cmp,
        u32 width,
        const u32* hashes, const K* keys,
        u32 hash, K key)
    {
        ASSERT(IsPow2(width));

        const u32 mask = width - 1u;
        for (u32 j = 0u; j < width; ++j)
        {
            const u32 i = (hash + j) & mask;
            const u32 storedHash = hashes[i];
            if (IsEmpty(storedHash))
            {
                return -1;
            }
            if (storedHash == hash)
            {
                if (cmp.Equals(key, keys[i]))
                {
                    return (i32)i;
                }
            }
        }
        return -1;
    }

    template<typename K>
    static i32 Find(
        const Comparator<K> cmp,
        u32 width,
        const u32* hashes, const K* keys,
        K key)
    {
        return Find<K>(
            cmp, width, hashes, keys, Hash(cmp, key), key);
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static i32 Insert(
        const Comparator<K> cmp,
        u32 width,
        u32* hashes, K* keys,
        u32 hash, K key)
    {
        ASSERT(IsPow2(width));

        const u32 mask = width - 1u;
        for (u32 j = 0u; j < width; ++j)
        {
            const u32 i = (hash + j) & mask;
            const u32 storedHash = hashes[i];
            if (IsEmptyOrTomb(storedHash))
            {
                hashes[i] = hash;
                keys[i] = key;
                return (i32)i;
            }
            if (storedHash == hash)
            {
                if (cmp.Equals(keys[i], key))
                {
                    return -1;
                }
            }
        }
        return -1;
    }

    template<typename K>
    static i32 Insert(
        const Comparator<K> cmp,
        u32 width,
        u32* hashes, K* keys,
        K key)
    {
        return Insert<K>(
            cmp, width, hashes, keys, Hash(cmp, key), key);
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static i32 Remove(
        const Comparator<K> cmp,
        u32 width,
        u32* hashes, K* keys,
        u32 hash, K key)
    {
        i32 i = Find(
            cmp, width, hashes, values, hash, value);
        if (i == -1)
        {
            return -1;
        }
        hashes[i] = TombHash;
        memset(values + i, 0, sizeof(K));
        return i;
    }

    template<typename K>
    static i32 Remove(
        const Comparator<K> cmp,
        u32 width,
        u32* hashes, K* keys,
        K key)
    {
        return Remove<K>(
            cmp, width, hashes, keys, Hash(cmp, key), key);
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static bool Contains(
        const Comparator<K> cmp,
        u32 width,
        const u32* hashes, const K* keys,
        K key)
    {
        return Find<K>(
            cmp, width, hashes, keys, key) != -1;
    }

    // ------------------------------------------------------------------------

    static u32 TrimWidth(u32 width, u32 count)
    {
        if (width > count)
        {
            if (count > 0u)
            {
                u32 cap = 1u;
                while (count > cap)
                {
                    cap <<= 1u;
                }
                if (cap < width)
                {
                    return cap;
                }
                return width;
            }
            else
            {
                return 0;
            }
        }
        return width;
    }

    static u32 GrowWidth(u32 width, u32 count)
    {
        if (((count + 1u) * 4u) >= (width * 3u))
        {
            return Max(32u, width * 2u);
        }
        return width;
    }
};

// ------------------------------------------------------------------------

template<
    typename K,
    const Comparator<K>& cmp>
struct HashSet
{
    u32* m_hashes;
    K* m_keys;
    u32 m_width;
    u32 m_count;
    AllocType m_allocator;

    // ------------------------------------------------------------------------

    inline i32 Size() const { return (i32)m_count; }
    inline i32 Capacity() const { return (i32)m_width; }

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

        u32* newHashes = Allocator::CallocT<u32>(m_allocator, width);
        K* newKeys = Allocator::CallocT<K>(m_allocator, width);

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
};

// ----------------------------------------------------------------------------

template<
    typename K,
    typename V,
    const Comparator<K>& cmp>
struct HashDict
{
    u32* m_hashes;
    K* m_keys;
    V* m_values;
    u32 m_width;
    u32 m_count;
    AllocType m_allocator;

    // ------------------------------------------------------------------------

    inline i32 Size() const { return (i32)m_count; }
    inline i32 Capacity() const { return (i32)m_width; }

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

        u32* newHashes = Allocator::CallocT<u32>(m_allocator, width);
        K* newKeys = Allocator::CallocT<K>(m_allocator, width);
        V* newValues = Allocator::AllocT<V>(m_allocator, width);

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
};
