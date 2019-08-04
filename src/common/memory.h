#pragma once

#include <stdint.h>
#include <string.h>

template<typename T>
inline void MemClear(T& t)
{
    memset(&t, 0, sizeof(T));
}

template<typename T>
inline void MemClear(T* t, int32_t size)
{
    memset(t, 0, sizeof(T) * size);
}

template<typename T>
inline void MemCpy(T& dst, const T& src)
{
    memcpy(&dst, &src, sizeof(T));
}

template<typename T>
inline void MemCpy(T* dst, const T* src, int32_t size)
{
    memcpy(dst, src, sizeof(T) * size);
}

template<typename T>
inline int32_t MemCmp(const T& a, const T& b)
{
    return memcmp(&a, &b, sizeof(T));
}

template<typename T>
inline int32_t MemCmp(const T* a, const T* b, int32_t size)
{
    return memcmp(a, b, sizeof(T) * size);
}
