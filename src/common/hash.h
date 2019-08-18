#pragma once

#include "common/int_types.h"

static constexpr u8 Fnv8_Bias = 251;
static constexpr u8 Fnv8_Prime = 83;
static constexpr u16 Fnv16_Bias = 65521;
static constexpr u16 Fnv16_Prime = 21841;
static constexpr u32 Fnv32_Bias = 2166136261;
static constexpr u32 Fnv32_Prime = 16777619;
static constexpr u64 Fnv64_Bias = 14695981039346656037;
static constexpr u64 Fnv64_Prime = 1099511628211;

inline constexpr u8 Fnv8Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u8 hash = Fnv8_Bias;
    for (usize i = 0u; i < bytes; ++i)
    {
        hash ^= octets[i];
        hash *= Fnv8_Prime;
    }
    return hash;
}

inline constexpr u16 Fnv16Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u16 hash = Fnv16_Bias;
    for (usize i = 0u; i < bytes; ++i)
    {
        hash ^= octets[i];
        hash *= Fnv16_Prime;
    }
    return hash;
}

inline constexpr u32 Fnv32Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u32 hash = Fnv32_Bias;
    for(usize i = 0u; i < bytes; ++i)
    {
        hash ^= octets[i];
        hash *= Fnv32_Prime;
    }
    return hash;
}

inline constexpr u32 Fnv32String(const char* const ptr)
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

inline constexpr u64 Fnv64Bytes(const void* const ptr, usize bytes)
{
    const u8* const octets = (const u8* const)ptr;
    u64 hash = Fnv64_Bias;
    for(usize i = 0u; i < bytes; ++i)
    {
        hash ^= octets[i];
        hash *= Fnv64_Prime;
    }
    return hash;
}

inline constexpr u64 Fnv64String(const char * const ptr)
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
