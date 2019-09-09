#pragma once

#include "common/int_types.h"

#pragma warning(disable : 4307)

static constexpr u8 Fnv8_Bias = 251;
static constexpr u8 Fnv8_Prime = 83;
static constexpr u16 Fnv16_Bias = 65521;
static constexpr u16 Fnv16_Prime = 21841;
static constexpr u32 Fnv32_Bias = 2166136261;
static constexpr u32 Fnv32_Prime = 16777619;
static constexpr u64 Fnv64_Bias = 14695981039346656037;
static constexpr u64 Fnv64_Prime = 1099511628211;

inline constexpr u8 Fnv8Byte(u8 x, u8 hash)
{
    return (hash ^ x) * Fnv8_Prime;
}

inline constexpr u8 Fnv8Word(u16 x, u8 hash)
{
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u8 Fnv8DWord(u32 x, u8 hash)
{
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u8 Fnv8QWord(u64 x, u8 hash)
{
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;

    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv8Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u8 Fnv8Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u8 hash = Fnv8_Bias;
    for (usize i = 0u; i < bytes; ++i)
    {
        hash = Fnv8Byte(octets[i], hash);
    }
    return hash;
}

inline constexpr u8 Fnv8String(cstrc ptr)
{
    u8 hash = Fnv8_Bias;
    for (usize i = 0u; ptr[i]; ++i)
    {
        hash = Fnv8Byte(ptr[i], hash);
    }
    return hash;
}

inline constexpr u16 Fnv16Byte(u8 x, u16 hash)
{
    return (hash ^ x) * Fnv16_Prime;
}

inline constexpr u16 Fnv16Word(u16 x, u16 hash)
{
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u16 Fnv16DWord(u32 x, u16 hash)
{
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u16 Fnv16QWord(u64 x, u16 hash)
{
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;

    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv16Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u16 Fnv16Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u16 hash = Fnv16_Bias;
    for (usize i = 0u; i < bytes; ++i)
    {
        hash = Fnv16Byte(octets[i], hash);
    }
    return hash;
}

inline constexpr u16 Fnv16String(cstrc ptr)
{
    u16 hash = Fnv16_Bias;
    for (usize i = 0u; ptr[i]; ++i)
    {
        hash = Fnv16Byte(ptr[i], hash);
    }
    return hash;
}

inline constexpr u32 Fnv32Byte(u8 x, u32 hash)
{
    return (hash ^ x) * Fnv32_Prime;
}

inline constexpr u32 Fnv32Word(u16 x, u32 hash)
{
    hash = Fnv32Byte(x & 0xff, hash);
    hash >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u32 Fnv32Dword(u32 x, u32 hash)
{
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u32 Fnv32Qword(u64 x, u32 hash)
{
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;

    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv32Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u32 Fnv32Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u32 hash = Fnv32_Bias;
    for(usize i = 0u; i < bytes; ++i)
    {
        hash = Fnv32Byte(octets[i], hash);
    }
    return hash;
}

inline constexpr u32 Fnv32String(cstrc ptr)
{
    u32 hash = Fnv32_Bias;
    for(u32 i = 0u; ptr[i]; ++i)
    {
        hash = Fnv32Byte(ptr[i], hash);
    }
    return hash;
}

inline constexpr u64 Fnv64Byte(u8 x, u64 hash)
{
    return (hash ^ x) * Fnv64_Prime;
}

inline constexpr u64 Fnv64Word(u16 x, u64 hash)
{
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u64 Fnv64Dword(u32 x, u64 hash)
{
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u64 Fnv64Qword(u64 x, u64 hash)
{
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;

    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    x >>= 8u;
    hash = Fnv64Byte(x & 0xff, hash);
    return hash;
}

inline constexpr u64 Fnv64Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u64 hash = Fnv64_Bias;
    for(usize i = 0u; i < bytes; ++i)
    {
        hash = Fnv64Byte(octets[i], hash);
    }
    return hash;
}

inline constexpr u64 Fnv64String(cstrc ptr)
{
    u64 hash = Fnv64_Bias;
    for(u32 i = 0u; ptr[i]; ++i)
    {
        hash = Fnv64Byte(ptr[i], hash);
    }
    return hash;
}
