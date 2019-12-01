#pragma once

#include "common/macro.h"
#include "common/int_types.h"

inline constexpr void Unroll16(i32 count, i32& n16, i32& n4, i32& n1)
{
    Assert(count >= 0);
    n16 = count >> 4;
    n4 = (count >> 2) & 3;
    n1 = count & 3;
}

inline constexpr void Unroll64(i32 count, i32& n64, i32& n8, i32& n1)
{
    Assert(count >= 0);
    n64 = count >> 6;
    n8 = (count >> 3) & 7;
    n1 = count & 7;
}
