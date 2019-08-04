#pragma once

#include <stdint.h>
#include <string.h>

template<typename T>
inline int32_t Find(const T* ptr, int32_t len, T key)
{
    for(int32_t i = 0; i < len; ++i)
    {
        if(memcmp(ptr + i, &key, sizeof(T)) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline int32_t Find(const T* ptr, int32_t len, T key, int32_t (*cmp)(T lhs, T rhs))
{
    for(int32_t i = 0; i < len; ++i)
    {
        if(cmp(ptr[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline int32_t RFind(const T* ptr, int32_t len, T key)
{
    for(int32_t i = len - 1; i >= 0; --i)
    {
        if(memcmp(ptr + i, &key, sizeof(T)) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline int32_t RFind(const T* ptr, int32_t len, T key, int32_t (*cmp)(T lhs, T rhs))
{
    for(int32_t i = len - 1; i >= 0; --i)
    {
        if(cmp(ptr[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}
