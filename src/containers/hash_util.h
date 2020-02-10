#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"

namespace HashUtil
{
    // ------------------------------------------------------------------------

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

    static constexpr u32 TombMask = 1u << 31u;
    static constexpr u32 HashMask = ~TombMask;

    static constexpr u32 IsFilled(u32 hash)
    {
        return hash & 1u;
    }

    static constexpr u32 IsTomb(u32 hash)
    {
        return (hash >> 31u) & 1u;
    }

    static constexpr u32 IsEmpty(u32 hash)
    {
        return IsFilled(~hash);
    }

    static constexpr u32 IsNotTomb(u32 hash)
    {
        return IsTomb(~hash);
    }

    static constexpr u32 IsEmptyOrTomb(u32 hash)
    {
        return IsEmpty(hash) | IsTomb(hash);
    }

    static constexpr u32 IsValidHash(u32 hash)
    {
        return IsFilled(hash) & IsNotTomb(hash);
    }

    static constexpr u32 CreateHash(u32 hash)
    {
        return (hash | 1u) & HashMask;
    }

    template<typename K>
    static u32 Hash(K key)
    {
        return CreateHash(Fnv32Bytes(&key, sizeof(K)));
    }
};
