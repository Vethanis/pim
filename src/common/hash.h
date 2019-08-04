#pragma once

#include "common/int_types.h"

inline u32 Fnv32(const u8* ptr, i32 len)
{
    u32 hash = 2166136261u;
    for(i32 i = 0; i < len; ++i)
    {
        hash ^= ptr[i];
        hash *= 16777619u;
    }
    return hash;
}

inline u32 Fnv32(cstr ptr)
{
    u32 hash = 2166136261u;
    while(*ptr)
    {
        hash ^= *ptr++;
        hash *= 16777619u;
    }
    return hash;
}

template<typename T>
inline u32 Fnv32(const T& x)
{
    return Fnv32((const u8*)&x, sizeof(T));
}

template<typename T>
inline u32 Fnv32(const T* ptr, i32 len)
{
    return Fnv32((const u8*)ptr, sizeof(T) * len);
}

inline u64 Fnv64(const u8* ptr, i32 len)
{
    u64 hash = 14695981039346656037ul;
    for(i32 i = 0; i < len; ++i)
    {
        hash ^= ptr[i];
        hash *= 1099511628211ul;
    }
    return hash;
}

inline u64 Fnv64(cstr ptr)
{
    u64 hash = 14695981039346656037ul;
    while(*ptr)
    {
        hash ^= *ptr++;
        hash *= 1099511628211ul;
    }
    return hash;
}

template<typename T>
inline u64 Fnv64(const T& x)
{
    return Fnv64((const u8*)&x, sizeof(T));
}

template<typename T>
inline u64 Fnv64(const T* ptr, i32 len)
{
    return Fnv64((const u8*)ptr, sizeof(T) * len);
}
