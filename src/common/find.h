#pragma once

#include "containers/slice.h"
#include "containers/array.h"

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
inline i32 Find(const Slice<const T> slice, const T& key)
{
    return Find(slice.begin(), slice.Size(), key);
}
template<typename T>
inline i32 Find(const Array<T> arr, const T& key)
{
    return Find(arr.begin(), arr.Size(), key);
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
inline i32 RFind(const Slice<const T> slice, const T& key)
{
    return RFind(slice.begin(), slice.Size(), key);
}
template<typename T>
inline i32 RFind(const Array<T> arr, const T& key)
{
    return RFind(arr.begin(), arr.Size(), key);
}

template<typename T>
inline bool Contains(const T* ptr, i32 count, const T& key)
{
    return RFind(ptr, count, key) != -1;
}
template<typename T>
inline bool Contains(const Slice<const T> slice, const T& key)
{
    return Contains(slice.begin(), slice.Size(), key);
}
template<typename T>
inline bool Contains(const Array<T> arr, const T& key)
{
    return Contains(arr.begin(), arr.Size(), key);
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
inline i32 FindMax(const Slice<const T> slice)
{
    return FindMax(slice.begin(), slice.Size());
}
template<typename T>
inline i32 FindMax(const Array<T> arr)
{
    return FindMax(arr.begin(), arr.Size());
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
template<typename T>
inline i32 FindMin(const Slice<const T> slice)
{
    return FindMin(slice.begin(), slice.Size());
}
template<typename T>
inline i32 FindMin(const Array<T> arr)
{
    return FindMin(arr.begin(), arr.Size());
}

template<typename T>
inline bool FindRemove(Array<T>& arr, const T& key)
{
    i32 i = RFind(arr, key);
    if (i != -1)
    {
        arr.Remove(i);
        return true;
    }
    return false;
}

template<typename T>
inline bool UniquePush(Array<T>& arr, const T& key)
{
    if (!Contains(arr, key))
    {
        arr.Grow() = key;
        return true;
    }
    return false;
}
