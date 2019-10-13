#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
inline i32 Find(const T* ptr, i32 count, const T& key)
{
    for (i32 i = 0; i < count; ++i)
    {
        if (ptr[i] == key)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline i32 RFind(const T* ptr, i32 count, const T& key)
{
    for (i32 i = count - 1; i >= 0; --i)
    {
        if (ptr[i] == key)
        {
            return i;
        }
    }
    return -1;
}

template<typename T>
inline i32 FindMax(const T* ptr, i32 count)
{
    if (count <= 0)
    {
        return -1;
    }
    i32 m = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (ptr[i] > ptr[m])
        {
            m = i;
        }
    }
    return m;
}

template<typename T>
inline i32 FindMin(const T* ptr, i32 count)
{
    if (count <= 0)
    {
        return -1;
    }
    i32 m = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (ptr[i] < ptr[m])
        {
            m = i;
        }
    }
    return m;
}
