#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include "os/thread.h"
#include "os/atomics.h"
#include <string.h>

template<typename K, typename V, const Comparator<K>& cmp>
struct HashDict
{
    mutable OS::RWLock m_lock;
    u32 m_count;
    u32 m_width;
    mutable OS::RWFlag* m_flags;
    u32* m_hashes;
    K* m_keys;
    V* m_values;
    AllocType m_allocator;

    // ------------------------------------------------------------------------

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

    // ------------------------------------------------------------------------

    i32 Find(K key) const
    {
        const u32 hash = HashUtil::Hash(cmp, key);
        OS::ReadGuard guard(m_lock);

        OS::RWFlag* flags = LoadPtr(m_flags);
        const u32* hashes = Load(m_hashes);
        const K* keys = Load(m_keys);
        const u32 width = Load(m_width);
        const u32 mask = width - 1u;

        u32 ct = width;
        u32 j = hash;
        while (ct--)
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
                if (cmp.Equals(key, keys[j]))
                {
                    return (i32)j;
                }
            }
            ++j;
        }

        return -1;
    }

    bool Contains(K key) const
    {
        return Find(key) != -1;
    }

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

        OS::RWFlag* oldFlags = LoadPtr(m_flags);
        u32* oldHashes = LoadPtr(m_hashes);
        K* oldKeys = LoadPtr(m_keys);
        V* oldValues = LoadPtr(m_values);
        const u32 oldWidth = capacity();
        const bool grow = newWidth > oldWidth;

        if (grow)
        {
            const u32 newMask = newWidth - 1u;
            for (u32 iSrc = 0u; iSrc < oldWidth; ++iSrc)
            {
                u32 iDst = oldHashes[iSrc];
                if (HashUtil::IsValidHash(iDst))
                {
                    while (true)
                    {
                        iDst &= newMask;
                        if (!newHashes[iDst])
                        {
                            newFlags[iDst] = oldFlags[iSrc];
                            newHashes[iDst] = oldHashes[iSrc];
                            newKeys[iDst] = oldKeys[iSrc];
                            newValues[iDst] = oldValues[iSrc];
                            break;
                        }
                        ++iDst;
                    }
                }
            }

            StorePtr(m_flags, newFlags);
            StorePtr(m_hashes, newHashes);
            StorePtr(m_keys, newKeys);
            StorePtr(m_values, newValues);
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

    bool Add(K key, V value)
    {
        Reserve(size() + 3u);

        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        OS::RWFlag* flags = LoadPtr(m_flags);
        u32* hashes = LoadPtr(m_hashes);
        K* keys = LoadPtr(m_keys);
        V* values = LoadPtr(m_values);

        u32 j = hash;
        u32 ct = capacity();
        const u32 mask = ct - 1u;

        while (ct--)
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
            ++j;
        }

        return false;
    }

    bool Remove(K key, V& valueOut)
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        OS::RWFlag* flags = LoadPtr(m_flags);
        u32* hashes = LoadPtr(m_hashes);
        const K* keys = LoadPtr(m_keys);
        const V* values = LoadPtr(m_values);

        u32 j = hash;
        u32 ct = capacity();
        const u32 mask = ct - 1u;

        while (ct--)
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
                if (cmp.Equals(key, keys[j]) && CmpExStrong(hashes[j], jHash, HashUtil::TombMask))
                {
                    valueOut = values[j];
                    memset(keys + j, 0, sizeof(K));
                    memset(values + j, 0, sizeof(V));
                    Dec(m_count);
                    return true;
                }
            }
            ++j;
        }

        return false;
    }

    bool Get(K key, V& valueOut) const
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        OS::RWFlag* flags = LoadPtr(m_flags);
        const u32* hashes = LoadPtr(m_hashes);
        const K* keys = LoadPtr(m_keys);
        const V* values = LoadPtr(m_values);

        u32 j = hash;
        u32 ct = capacity();
        const u32 mask = ct - 1u;

        while (ct--)
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
                if (cmp.Equals(key, keys[j]))
                {
                    valueOut = values[j];
                    return true;
                }
            }
            ++j;
        }

        return false;
    }

    bool Set(K key, const V& valueIn)
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        OS::RWFlag* flags = LoadPtr(m_flags);
        const u32* hashes = LoadPtr(m_hashes);
        const K* keys = LoadPtr(m_keys);
        V* values = LoadPtr(m_values);

        u32 j = hash;
        u32 ct = capacity();
        const u32 mask = ct - 1u;

        while (ct--)
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
                if (cmp.Equals(key, keys[j]))
                {
                    values[j] = valueIn;
                    return true;
                }
            }
            ++j;
        }

        return false;
    }
};
