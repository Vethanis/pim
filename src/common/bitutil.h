#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include <intrin.h>

namespace BitUtil
{
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

    static u32 ToPow2(u32 x)
    {
        unsigned long bit = 0u;
        _BitScanReverse(&bit, x - 1u);
        u32 result = 1u << (bit + 1u);
        ASSERT(IsPow2(result));
        return result;
    }
};
