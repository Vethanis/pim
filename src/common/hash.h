#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/unroll.h"

#pragma warning(disable : 4307)

// ----------------------------------------------------------------------------
// Fnv32

static constexpr u32 Fnv32Bias = 2166136261;
static constexpr u32 Fnv32Prime = 16777619;

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

inline constexpr u32 Fnv32String(cstrc ptr, u32 hash = Fnv32Bias)
{
    for (i32 i = 0; ptr[i]; ++i)
    {
        hash = Fnv32Byte(ptr[i], hash);
    }
    return hash;
}

inline u32 Fnv32Bytes(const void* ptr, i32 nBytes, u32 hash = Fnv32Bias)
{
    i32 n16 = 0, n4 = 0, n1 = 0;
    Unroll16(nBytes, n16, n4, n1);

    const u64* p8s = (const u64*)ptr;
    for (i32 i = 0; i < n16; ++i)
    {
        hash = Fnv32Qword(*p8s++, hash);
        hash = Fnv32Qword(*p8s++, hash);
    }

    const u32* p4s = (const u32*)p8s;
    for (i32 i = 0; i < n4; ++i)
    {
        hash = Fnv32Dword(*p4s++);
    }

    const u8* p1s = (const u8*)p4s;
    for(i32 i = 0; i < n1; ++i)
    {
        hash = Fnv32Byte(*p1s++);
    }

    return hash;
}

template<typename T>
inline u32 Fnv32T(const T* ptr, i32 count, u32 hash = Fnv32Bias)
{
    return Fnv32Bytes(ptr, sizeof(T) * count, hash);
}

// ----------------------------------------------------------------------------
// Fnv64

static constexpr u64 Fnv64Bias = 14695981039346656037;
static constexpr u64 Fnv64Prime = 1099511628211;

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
    for (i32 i = 0; ptr[i]; ++i)
    {
        hash = Fnv64Byte(ptr[i], hash);
    }
    return hash;
}

inline u64 Fnv64Bytes(const void* ptr, i32 nBytes, u64 hash = Fnv64Bias)
{
    i32 n16 = 0, n4 = 0, n1 = 0;
    Unroll16(nBytes, n16, n4, n1);

    const u64* p8s = (const u64*)ptr;
    for (i32 i = 0; i < n16; ++i)
    {
        hash = Fnv64Qword(*p8s++, hash);
        hash = Fnv64Qword(*p8s++, hash);
    }

    const u32* p4s = (const u32*)p8s;
    for (i32 i = 0; i < n4; ++i)
    {
        hash = Fnv64Dword(*p4s++);
    }

    const u8* p1s = (const u8*)p4s;
    for (i32 i = 0; i < n1; ++i)
    {
        hash = Fnv64Byte(*p1s++);
    }

    return hash;
}

template<typename T>
inline u64 Fnv64T(const T* ptr, i32 count, u64 hash = Fnv64Bias)
{
    return Fnv64Bytes(ptr, sizeof(T) * count, hash);
}
