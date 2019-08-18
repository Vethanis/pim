#pragma once

#include "common/int_types.h"
#include "common/memory.h"

template<typename T>
inline i32 Find(const T* ptr, i32 len, T key)
{
    for(i32 i = 0; i < len; ++i)
    {
        if(MemCmp(ptr + i, &key, sizeof(T)) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline i32 Find(const T* ptr, i32 len, T key, i32 (*cmp)(T lhs, T rhs))
{
    for(i32 i = 0; i < len; ++i)
    {
        if(cmp(ptr[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline i32 RFind(const T* ptr, i32 len, T key)
{
    for(i32 i = len - 1; i >= 0; --i)
    {
        if(MemCmp(ptr + i, &key, sizeof(T)) == 0)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline i32 RFind(const T* ptr, i32 len, T key, i32 (*cmp)(T lhs, T rhs))
{
    for(i32 i = len - 1; i >= 0; --i)
    {
        if(cmp(ptr[i], key) == 0)
        {
            return i;
        }
    }
    return -1;
}
