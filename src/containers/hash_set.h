#pragma once

#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "os/atomics.h"
#include "os/thread.h"
#include <string.h>

template<
    typename K,
    const Comparator<K>& cmp>
    struct HashSet
{
    OS::RWLock m_lock;
    OS::RWFlag* m_flags;
    u32* m_hashes;
    K* m_keys;
    u32 m_count;
    u32 m_width;
    AllocType m_allocator;

    // ------------------------------------------------------------------------

    u32 size() const { return Load(m_count); }
    u32 capacity() const { return Load(m_width); }
    AllocType GetAllocator() const { return m_allocator; }

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
        m_flags = 0;
        m_hashes = 0;
        m_keys = 0;
        m_width = 0;
        m_count = 0;
        m_lock.Close();
    }

    void Clear()
    {
        OS::WriteGuard guard(m_lock);
        m_count = 0;
        if (m_hashes)
        {
            memset(m_flags, 0, sizeof(OS::RWFlag) * m_width);
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

    i32 Find(K key) const
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        const OS::RWFlag* flags = LoadPtr(m_flags);
        const u32* hashes = LoadPtr(m_hashes);
        const K* keys = LoadPtr(m_keys);
        const u32 width = capacity();
        const u32 mask = width - 1u;

        u32 j = hash;
        u32 ct = width;
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
        const u32 newWidth = HashUtil::ToPow2(minCount);
        if (newWidth <= capacity())
        {
            return;
        }

        const AllocType allocator = m_allocator;
        OS::RWFlag* newFlags = Allocator::CallocT<OS::RWFlag>(allocator, newWidth);
        u32* newHashes = Allocator::CallocT<u32>(allocator, newWidth);
        K* newKeys = Allocator::CallocT<K>(allocator, newWidth);

        m_lock.LockWriter();

        OS::RWFlag* oldFlags = m_flags;
        u32* oldHashes = m_hashes;
        K* oldKeys = m_keys;
        const u32 oldWidth = capacity();
        const u32 oldMask = oldWidth - 1u;
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
                            break;
                        }
                        ++j;
                    }
                }
            }

            m_flags = newFlags;
            m_hashes = newHashes;
            m_keys = newKeys;
            Store(m_width, newWidth);
        }

        m_lock.UnlockWriter();

        if (grow)
        {
            Allocator::Free(oldFlags);
            Allocator::Free(oldHashes);
            Allocator::Free(oldKeys);
        }
        else
        {
            Allocator::Free(newFlags);
            Allocator::Free(newHashes);
            Allocator::Free(newKeys);
        }
    }

    bool Add(K key)
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        Reserve(size() + 3u);
        OS::ReadGuard guard(m_lock);

        u32 width = capacity();
        const u32 mask = width - 1u;
        OS::RWFlag* flags = m_flags;
        u32* hashes = m_hashes;
        K* keys = m_keys;

        for(u32 j = hash; width--; ++j)
        {
            j &= mask;
            u32 prev = Load(hashes[j]);
            if (HashUtil::IsEmptyOrTomb(prev))
            {
                OS::WriteFlagGuard g2(flags[j]);
                if (CmpExStrong(hashes[j], prev, hash))
                {
                    keys[j] = key;
                    Inc(m_count);
                    return true;
                }
            }
        }

        return false;
    }

    bool Remove(K key)
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        OS::RWFlag* const flags = m_flags;
        u32* const hashes = m_hashes;
        K* const keys = m_keys;

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
                if (cmp.Equals(key, keys[j]) && CmpExStrong(hashes[j], jHash, HashUtil::TombMask))
                {
                    memset(keys + j, 0, sizeof(K));
                    Dec(m_count);
                    return true;
                }
            }
        }

        return false;
    }

    bool Get(K key, K& keyOut) const
    {
        const u32 hash = HashUtil::Hash(cmp, key);

        OS::ReadGuard guard(m_lock);

        const OS::RWFlag* const flags = m_flags;
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;

        u32 width = capacity();
        const u32 mask = width - 1u;

        for (u32 j = hash; width--; ++j)
        {
            j &= mask;
            u32 prev = Load(hashes[j]);
            if (HashUtil::IsEmpty(prev))
            {
                break;
            }
            if (prev == hash)
            {
                OS::ReadFlagGuard g2(flags[j]);
                if (cmp.Equals(key, keys[j]))
                {
                    keyOut = keys[j];
                    return true;
                }
            }
        }

        return false;
    }

    void GetElements(Array<K>& keysOut)
    {
        keysOut.Clear();
        keysOut.Reserve((i32)size());

        OS::ReadGuard guard(m_lock);

        const OS::RWFlag* const flags = m_flags;
        const u32* const hashes = m_hashes;
        const K* const keys = m_keys;
        const u32 width = capacity();

        for (u32 j = 0u; j < width; ++j)
        {
            OS::ReadFlagGuard g2(flags[j]);
            if (HashUtil::IsValidHash(Load(hashes[j])))
            {
                keysOut.PushBack(keys[j]);
            }
        }
    }
};
