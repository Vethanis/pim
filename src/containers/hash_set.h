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
    static u32 Hash(const IComparator<K>& cmp, K key)
    {
        u32 hash = cmp.Hash(key);
        hash = IsEmpty(hash) ? (hash + 1u) : hash;
        hash = IsTomb(hash) ? (hash - 1u) : hash;
        return hash;
    }

    // ------------------------------------------------------------------------

    template<typename K>
    static i32 Find(
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
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
        const IComparator<K>& cmp,
        u32 width,
        const u32* hashes, const K* keys,
        K key)
    {
        return Find<K>(
            cmp, width, hashes, keys, key) != -1;
    }

    // ------------------------------------------------------------------------

    using OnInsertFn = void(*)(void* userData, i32 iOld, i32 iNew);

    struct ResizeInput
    {
        AllocType allocator;
        u32 width;
        u32* hashes;
        void* keys;
        OnInsertFn onInsert;
        void* userData;
    };

    template<typename K>
    static void Resize(
        const IComparator<K>& cmp,
        ResizeInput& input,
        u32 newWidth)
    {
        ASSERT(IsPow2(input.width));
        ASSERT(IsPow2(newWidth));

        const AllocType allocator = input.allocator;
        u32* newHashes = (u32*)Allocator::Alloc(
            allocator, sizeof(u32) * newWidth);
        K* newKeys = (K*)Allocator::Alloc(
            allocator, sizeof(K) * newWidth);
        memset(newHashes, 0, sizeof(u32) * newWidth);
        memset(newKeys, 0, sizeof(K) * newWidth);

        const OnInsertFn onInsert = input.onInsert;
        void* userData = input.userData;

        const u32* oldHashes = input.hashes;
        const K* oldKeys = (K*)input.keys;
        const u32 oldWidth = input.width;

        for (u32 i = 0u; i < oldWidth; ++i)
        {
            const u32 hash = oldHashes[i];
            if (IsEmptyOrTomb(hash))
            {
                continue;
            }
            i32 j = Insert(
                cmp,
                newWidth,
                newHashes,
                newKeys,
                hash,
                oldKeys[i]);
            ASSERT(j != -1);
            if (onInsert != nullptr)
            {
                onInsert(userData, i, j);
            }
        }

        Allocator::Free(input.hashes);
        Allocator::Free(input.keys);

        input.width = width;
        input.hashes = newHashes;
        input.keys = newKeys;
    }

    // ------------------------------------------------------------------------
};

template<typename K, const IComparator<K>& cmp>
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
        ASSERT(width >= m_count);

        HashUtil::ResizeInput input = {};
        input.allocator = m_allocator;
        input.width = m_width;
        input.hashes = m_hashes;
        input.keys = m_keys;

        HashUtil::Resize<K>(
            cmp,
            input,
            width);

        m_hashes = input.hashes;
        m_keys = (K*)input.keys;
        m_width = width;
    }

    void Trim()
    {
        const u32 count = m_count;
        const u32 width = m_width;
        if (width > count)
        {
            if (count)
            {
                u32 cap = 1;
                while (count > cap)
                {
                    cap <<= 1;
                }
                if (cap < width)
                {
                    Resize(cap);
                }
            }
            else
            {
                Reset();
            }
        }
    }

    bool Add(K key)
    {
        if ((m_count + 1) >= m_width)
        {
            Resize(Max(16, m_width * 2));
        }
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

template<typename K, typename V, const IComparator<K>& cmp>
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
        Allocator::Free(m_hashes);
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
        ASSERT(width >= m_count);

        V* newValues = (V*)Allocator::Alloc(m_allocator, sizeof(V) * width);
        memset(newValues, 0, sizeof(V) * width);

        HashUtil::ResizeInput input = {};
        input.allocator = m_allocator;
        input.width = m_width;
        input.hashes = m_hashes;
        input.keys = m_keys;

        struct Values
        {
            V* pOld;
            V* pNew;
        };
        Values values = { m_values, newValues };

        input.userData = &values;
        input.onInsert =
        [](void* ptr, i32 iOld, i32 iNew)
        {
            Values* values = (Values*)ptr;
            values->pNew[iNew] = values->pOld[iOld];
        };

        HashUtil::Resize<K>(cmp, input, width);

        m_width = input.width;
        m_hashes = input.hashes;
        m_keys = input.keys;

        Allocator::Free(m_values);
        m_values = newValues;
    }

    void Trim()
    {
        if (m_width > m_count)
        {
            const u32 ct = m_count;
            if (ct)
            {
                u32 cap = 1;
                while (ct > cap)
                {
                    cap <<= 1;
                }
                if (cap < m_width)
                {
                    Resize(cap);
                }
            }
            else
            {
                Reset();
            }
        }
    }

    void EnsureWidth()
    {
        const u32 width = m_width;
        const u32 count = m_count;
        if ((count + 1) >= width)
        {
            Resize(Max(16, width * 2));
        }
    }

    inline bool Add(K key, V value)
    {
        EnsureWidth();
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

    inline V& operator[](K key)
    {
        const u32 hash = HashUtil::Hash<K>(cmp, key);
        i32 i = HashUtil::Find<K>(
            cmp, m_width, m_hashes, m_keys, hash, key);
        if (i == -1)
        {
            EnsureWidth();
            i = HashUtil::Insert<K>(
                cmp, m_width, m_hashes, m_keys, hash, key);
            ASSERT(i != -1);
            memset(m_values + i, 0, sizeof(V));
        }
        return m_values[i];
    }
};
