#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "os/thread.h"
#include "os/atomics.h"
#include <string.h>

template<typename K, typename V>
struct HashDict
{
private:
    OS::RWLock m_lock;
    u32 m_count;
    u32 m_width;
    OS::RWFlag* m_flags;
    u32* m_hashes;
    K* m_keys;
    V* m_values;
    AllocType m_allocator;

public:
    AllocType GetAllocator() const { return m_allocator; }
    u32 size() const { return Load(m_count); }
    u32 capacity() const { return Load(m_width); }

    void Init(AllocType allocator)
    {
        memset(this, 0, sizeof(*this));
        m_allocator = allocator;
        m_lock.Open();
    }

    void Reset()
    {
        m_lock.LockWriter();
        Allocator::Free(m_flags);
        Allocator::Free(m_hashes);
        Allocator::Free(m_keys);
        Allocator::Free(m_values);
        m_flags = 0;
        m_hashes = 0;
        m_values = 0;
        m_width = 0;
        m_count = 0;
        m_lock.Close();
    }

    void Clear()
    {
        OS::WriteGuard guard(m_lock);
        m_count = 0;
        OS::RWFlag* flags = m_flags;
        u32* hashes = m_hashes;
        K* keys = m_keys;
        V* values = m_values;
        const u32 width = m_width;
        if (flags)
        {
            memset(flags, 0, sizeof(OS::RWFlag) * width);
            memset(hashes, 0, sizeof(u32) * width);
            memset(keys, 0, sizeof(K) * width);
            memset(values, 0, sizeof(V) * width);
        }
    }

    static HashDict Build(AllocType allocator, Slice<const K> keys, Slice<const V> values)
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
        if (newWidth <= capacity())
        {
            return;
        }

        const AllocType allocator = GetAllocator();
        OS::RWFlag* newFlags = Allocator::CallocT<OS::RWFlag>(allocator, newWidth);
        u32* newHashes = Allocator::CallocT<u32>(allocator, newWidth);
        K* newKeys = Allocator::CallocT<K>(allocator, newWidth);
        V* newValues = Allocator::AllocT<V>(allocator, newWidth);

        m_lock.LockWriter();

        OS::RWFlag* oldFlags = m_flags;
        u32* oldHashes = m_hashes;
        K* oldKeys = m_keys;
        V* oldValues = m_values;
        const u32 oldWidth = capacity();
        const bool grow = newWidth > oldWidth;

        if (grow)
        {
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
                            newFlags[j] = oldFlags[i];
                            newKeys[j] = oldKeys[i];
                            newValues[j] = oldValues[i];
                            break;
                        }
                        ++j;
                    }
                }
            }

            m_flags = newFlags;
            m_hashes = newHashes;
            m_keys = newKeys;
            m_values = newValues;
            Store(m_width, newWidth);
        }

        m_lock.UnlockWriter();

        if (grow)
        {
            Allocator::Free(oldFlags);
            Allocator::Free(oldHashes);
            Allocator::Free(oldKeys);
            Allocator::Free(oldValues);
        }
        else
        {
            Allocator::Free(newFlags);
            Allocator::Free(newHashes);
            Allocator::Free(newKeys);
            Allocator::Free(newValues);
        }
    }

    bool Contains(K key) const
    {
        V tmp;
        return Get(key, tmp);
    }

    bool Add(K key, const V& value)
    {
        if (Contains(key))
        {
            return false;
        }

        Reserve(size() + 3u);
        const u32 hash = HashUtil::Hash(key);
        OS::ReadGuard guard(m_lock);

        OS::RWFlag* const flags = m_flags;
        u32* const hashes = m_hashes;
        K* const keys = m_keys;
        V* const values = m_values;

        u32 width = capacity();
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            u32 jHash = Load(hashes[j]);
            if (HashUtil::IsEmptyOrTomb(jHash))
            {
                OS::WriteFlagGuard g2(flags[j]);
                if (CmpExStrong(hashes[j], jHash, hash))
                {
                    keys[j] = key;
                    values[j] = value;
                    Inc(m_count);
                    return true;
                }
            }
        }

        return false;
    }

    bool Remove(K key, V& valueOut)
    {
        const u32 hash = HashUtil::Hash(key);
        OS::ReadGuard guard(m_lock);

        OS::RWFlag* const flags = m_flags;
        u32* const hashes = m_hashes;
        const K* const keys = m_keys;
        const V* const values = m_values;

        u32 width = capacity();
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            u32 jHash = Load(hashes[j]);
            if (HashUtil::IsEmpty(jHash))
            {
                break;
            }
            if (jHash == hash)
            {
                OS::WriteFlagGuard g2(flags[j]);
                if ((key == keys[j]) && CmpExStrong(hashes[j], jHash, HashUtil::TombMask))
                {
                    valueOut = values[j];
                    memset(keys + j, 0, sizeof(K));
                    memset(values + j, 0, sizeof(V));
                    Dec(m_count);
                    return true;
                }
            }
        }

        return false;
    }

    bool Get(K key, V& valueOut) const
    {
        const u32 hash = HashUtil::Hash(key);
        OS::ReadGuard guard(m_lock);

        OS::RWFlag* const flags = m_flags;
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;
        const V* const values = m_values;

        u32 width = capacity();
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            const u32 jHash = Load(hashes[j]);
            if (HashUtil::IsEmpty(jHash))
            {
                break;
            }
            if (jHash == hash)
            {
                OS::ReadFlagGuard g2(flags[j]);
                if (key == keys[j])
                {
                    valueOut = values[j];
                    return true;
                }
            }
        }

        return false;
    }

    bool Set(K key, const V& valueIn)
    {
        const u32 hash = HashUtil::Hash(key);
        OS::ReadGuard guard(m_lock);

        OS::RWFlag* const flags = m_flags;
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;
        V* const values = m_values;

        u32 width = capacity();
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            const u32 jHash = Load(hashes[j]);
            if (HashUtil::IsEmpty(jHash))
            {
                break;
            }
            if (jHash == hash)
            {
                OS::WriteFlagGuard g2(flags[j]);
                if (key == keys[j])
                {
                    values[j] = valueIn;
                    return true;
                }
            }
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
            OS::ReadGuard guard(m_dict.m_lock);
            const u32* const hashes = m_dict.m_hashes;
            const u32 width = m_dict.capacity();
            u32 i = m_index;
            for (; i < width; ++i)
            {
                if (HashUtil::IsValidHash(Load(hashes[i])))
                {
                    break;
                }
            }
            m_index = i;
        }

        Pair operator*() const
        {
            const u32 i = m_index;
            OS::ReadGuard guard(m_dict.m_lock);
            ASSERT(i < m_dict.capacity());
            OS::ReadFlagGuard g2(m_dict.m_flags[i]);
            return { m_dict.m_keys[i], m_dict.m_values[i] };
        }

        bool operator!=(const_iterator rhs) const
        {
            return m_index != rhs.m_index;
        }

        const_iterator& operator++()
        {
            OS::ReadGuard guard(m_dict.m_lock);
            const u32* const hashes = m_dict.m_hashes;
            const u32 width = m_dict.capacity();
            u32 i = ++m_index;
            for (; i < width; ++i)
            {
                if (HashUtil::IsValidHash(Load(hashes[i])))
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
