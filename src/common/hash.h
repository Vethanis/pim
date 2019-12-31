#pragma once

#include "common/macro.h"
#include "common/int_types.h"

#pragma warning(disable : 4307)

// ----------------------------------------------------------------------------
// Fnv32

static constexpr u32 Fnv32Bias = 2166136261;
static constexpr u32 Fnv32Prime = 16777619;

inline constexpr char FnvToUpper(char x)
{
    i32 c = (i32)x;
    i32 lo = ('a' - 1) - c;     // c >= 'a'
    i32 hi = c - ('z' + 1);     // c <= 'z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

inline constexpr u32 Fnv32Char(char x, u32 hash = Fnv32Bias)
{
    return (hash ^ FnvToUpper(x)) * Fnv32Prime;
}

inline constexpr u32 Fnv32Byte(u8 x, u32 hash = Fnv32Bias)
{
    return (hash ^ x) * Fnv32Prime;
}

inline constexpr u32 Fnv32Word(u16 x, u32 hash = Fnv32Bias)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv32Prime;
    return hash;
}

inline constexpr u32 Fnv32Dword(u32 x, u32 hash = Fnv32Bias)
{
    hash = (hash ^ (0xff & (x >>  0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >>  8))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv32Prime;
    return hash;
}

inline constexpr u32 Fnv32Qword(u64 x, u32 hash = Fnv32Bias)
{
    hash = (hash ^ (0xff & (x >>  0))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >>  8))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv32Prime;

    hash = (hash ^ (0xff & (x >> 32))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 40))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 48))) * Fnv32Prime;
    hash = (hash ^ (0xff & (x >> 56))) * Fnv32Prime;
    return hash;
}

inline constexpr u32 Fnv32String(cstrc str, u32 hash = Fnv32Bias)
{
    ASSERT(str);
    for (i32 i = 0; str[i]; ++i)
    {
        hash = Fnv32Char(str[i], hash);
    }
    return hash;
}

inline u32 Fnv32Bytes(const void* ptr, i32 nBytes, u32 hash = Fnv32Bias)
{
    ASSERT(ptr || !nBytes);
    ASSERT(nBytes >= 0);
    const u8* asBytes = (const u8*)ptr;
    for(i32 i = 0; i < nBytes; ++i)
    {
        hash = Fnv32Byte(asBytes[i]);
    }
    return hash;
}

// ----------------------------------------------------------------------------
// Fnv64

static constexpr u64 Fnv64Bias = 14695981039346656037;
static constexpr u64 Fnv64Prime = 1099511628211;

inline constexpr u64 Fnv64Char(char x, u64 hash = Fnv64Bias)
{
    return (hash ^ FnvToUpper(x)) * Fnv64Prime;
}

inline constexpr u64 Fnv64Byte(u8 x, u64 hash = Fnv64Bias)
{
    return (hash ^ x) * Fnv64Prime;
}

inline constexpr u64 Fnv64Word(u16 x, u64 hash = Fnv64Bias)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    return hash;
}

inline constexpr u64 Fnv64Dword(u32 x, u64 hash = Fnv64Bias)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv64Prime;
    return hash;
}

inline constexpr u64 Fnv64Qword(u64 x, u64 hash = Fnv64Bias)
{
    hash = (hash ^ (0xff & (x >> 0))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 8))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 16))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 24))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 32))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 40))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 48))) * Fnv64Prime;
    hash = (hash ^ (0xff & (x >> 56))) * Fnv64Prime;
    return hash;
}

inline constexpr u64 Fnv64String(cstrc ptr, u64 hash = Fnv64Bias)
{
    ASSERT(ptr);
    for (i32 i = 0; ptr[i]; ++i)
    {
        hash = Fnv64Char(ptr[i], hash);
    }
    return hash;
}

inline u64 Fnv64Bytes(const void* ptr, i32 nBytes, u64 hash = Fnv64Bias)
{
    ASSERT(ptr || !nBytes);
    ASSERT(nBytes >= 0);
    const u8* asBytes = (const u8*)ptr;
    for (i32 i = 0; i < nBytes; ++i)
    {
        hash = Fnv64Byte(asBytes[i]);
    }
    return hash;
}
