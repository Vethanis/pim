#pragma once

#include "common/int_types.h"

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 Find(const T* const ptr, i32 count, const T& key)
{
    for (i32 i = 0; i < count; ++i)
    {
        if (key == ptr[i])
        {
            return i;
        }
    }
    return -1;
}
template<typename C, typename T>
inline i32 Find(const C x, const T& key)
{
    return Find(x.begin(), x.size(), key);
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 RFind(const T* const ptr, i32 count, const T& key)
{
    for (i32 i = count - 1; i >= 0; --i)
    {
        if (key == ptr[i])
        {
            return i;
        }
    }
    return -1;
}
template<typename C, typename T>
inline i32 RFind(const C x, const T& key)
{
    return RFind(x.begin(), x.size(), key);
}
// ----------------------------------------------------------------------------

template<typename T>
inline constexpr bool Contains(const T* const ptr, i32 count, const T& key)
{
    return RFind(ptr, count, key) != -1;
}
template<typename C, typename T>
inline bool Contains(const C x, const T& key)
{
    return Contains(x.begin(), x.size(), key);
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 FindMax(const T* const ptr, i32 count)
{
    if (count <= 0)
    {
        return -1;
    }
    i32 j = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (ptr[j] < ptr[i])
        {
            j = i;
        }
    }
    return j;
}
template<typename C>
inline i32 FindMax(const C x)
{
    return FindMax(x.begin(), x.size());
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 FindMin(const T* const ptr, i32 count)
{
    if (count <= 0)
    {
        return -1;
    }
    i32 j = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (ptr[i] < ptr[j])
        {
            j = i;
        }
    }
    return j;
}
template<typename C>
inline i32 FindMin(const C x)
{
    return FindMin(x.begin(), x.size());
}

// ----------------------------------------------------------------------------
