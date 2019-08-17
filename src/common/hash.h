#pragma once

#include "common/int_types.h"

static constexpr u32 Fnv32_Bias = 2166136261u;
static constexpr u32 Fnv32_Prime = 16777619u;
static constexpr u64 Fnv64_Bias = 14695981039346656037ul;
static constexpr u64 Fnv64_Prime = 1099511628211ul;

inline constexpr u32 Fnv32Bytes(const u8 * const ptr, u32 len)
{
    u32 hash = Fnv32_Bias;
    for(u32 i = 0u; i < len; ++i)
    {
        hash ^= ptr[i];
        hash *= Fnv32_Prime;
    }
    return hash;
}

inline constexpr u32 Fnv32String(cstrc ptr)
{
    u32 hash = Fnv32_Bias;
    for(u32 i = 0u; ptr[i]; ++i)
    {
        hash ^= ptr[i];
        hash *= Fnv32_Prime;
    }
    return hash;
}

inline constexpr u32 Fnv32Dword(u32 x, u32 hash = Fnv32_Bias)
{
    for (u32 i = 0u; i < 4u; ++i)
    {
        hash ^= x & 0xff;
        hash *= Fnv32_Prime;
        x >>= 8u;
    }
    return hash;
}

inline constexpr u32 Fnv32Qword(u64 x, u32 hash = Fnv32_Bias)
{
    for (u32 i = 0u; i < 8u; ++i)
    {
        hash ^= x & 0xff;
        hash *= Fnv32_Prime;
        x >>= 8u;
    }
    return hash;
}

inline constexpr u64 Fnv64Bytes(const u8 * const ptr, u32 len)
{
    u64 hash = Fnv64_Bias;
    for(u32 i = 0u; i < len; ++i)
    {
        hash ^= ptr[i];
        hash *= Fnv64_Prime;
    }
    return hash;
}

inline constexpr u64 Fnv64String(cstrc ptr)
{
    u64 hash = Fnv64_Bias;
    for(u32 i = 0u; ptr[i]; ++i)
    {
        hash ^= ptr[i];
        hash *= Fnv64_Prime;
    }
    return hash;
}

inline constexpr u64 Fnv64Dword(u32 x, u64 hash = Fnv64_Bias)
{
    for (u32 i = 0u; i < 4u; ++i)
    {
        hash ^= x & 0xff;
        hash *= Fnv64_Prime;
        x >>= 8u;
    }
    return hash;
}

inline constexpr u64 Fnv64Qword(u64 x, u64 hash = Fnv64_Bias)
{
    for (u32 i = 0u; i < 8u; ++i)
    {
        hash ^= x & 0xff;
        hash *= Fnv64_Prime;
        x >>= 8u;
    }
    return hash;
}
