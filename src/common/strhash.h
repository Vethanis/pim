#pragma once

#include "common/hash.h"
#include "common/stringutil.h"

inline constexpr u32 StrHash32(cstrc str, u32 hash = Fnv32Bias)
{
    if (!str || !str[0]) { return 0; }

    for (i32 i = 0; str[i]; ++i)
    {
        hash = Fnv32Byte(ToLower(str[i]), hash);
    }

    return hash ? hash : 1;
}

inline constexpr u64 StrHash64(cstrc str, u64 hash = Fnv64Bias)
{
    if (!str || !str[0]) { return 0; }

    for (i32 i = 0; str[i]; ++i)
    {
        hash = Fnv64Byte(ToLower(str[i]), hash);
    }

    return hash ? hash : 1;
}
