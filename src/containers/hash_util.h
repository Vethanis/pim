#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/comparator.h"

namespace HashUtil
{
    static constexpr u32 EmptyHash = 0x0u;
    static constexpr u32 TombHash = 0xffffffffu;

    // ------------------------------------------------------------------------

    static constexpr u32 RotL(u32 x, i32 shift)
    {
        return (x << shift) | (x >> (32 - shift));
    }

    static constexpr u32 RotR(u32 x, i32 shift)
    {
        return (x >> shift) | (x << (32 - shift));
    }

    static constexpr bool IsPow2(u32 x)
    {
        return (x & (x - 1u)) == 0u;
    }

    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    static constexpr u32 ToPow2(u32 x)
    {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        ++x;
        return x;
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

    static u32 Iterate(const u32* const hashes, const u32 width, u32 i)
    {
        u32 j = i + 1;
        for (; j < width; ++j)
        {
            if (!IsEmptyOrTomb(hashes[j]))
            {
                break;
            }
        }
        return j;
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
            cmp, width, hashes, keys, hash, key);
        if (i == -1)
        {
            return -1;
        }
        hashes[i] = TombHash;
        keys[i] = {};
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
};
