#pragma once

#include "common/comparator.h"
#include "containers/slice.h"
#include "containers/array.h"

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 Find(const T* const ptr, i32 count, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    for (i32 i = 0; i < count; ++i)
    {
        if (cmp.Equals(key, ptr[i]))
        {
            return i;
        }
    }
    return -1;
}
template<typename T>
inline i32 Find(const Slice<const T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return Find(x.begin(), x.Size(), key, cmp);
}
template<typename T>
inline i32 Find(const Array<T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return Find(x.begin(), x.Size(), key, cmp);
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 RFind(const T* const ptr, i32 count, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    for (i32 i = count - 1; i >= 0; --i)
    {
        if (cmp.Equals(key, ptr[i]))
        {
            return i;
        }
    }
    return -1;
}
template<typename T>
inline i32 RFind(const Slice<const T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return RFind(x.begin(), x.Size(), key, cmp);
}
template<typename T>
inline i32 RFind(const Array<T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return RFind(x.begin(), x.Size(), key, cmp);
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr bool Contains(const T* const ptr, i32 count, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return RFind(ptr, count, key, cmp) != -1;
}
template<typename T>
inline bool Contains(const Slice<const T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return Contains(x.begin(), x.Size(), key, cmp);
}
template<typename T>
inline bool Contains(const Array<T> x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    return Contains(x.begin(), x.Size(), key, cmp);
}

// ----------------------------------------------------------------------------

template<typename T>
inline bool FindRemove(T* const ptr, i32& count, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    i32 i = RFind(ptr, count, key, cmp);
    if (i != -1)
    {
        i32 back = --count;
        ptr[i] = ptr[back];
        return true;
    }
    return false;
}

template<typename T>
inline bool FindRemove(Array<T>& x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    i32 i = RFind(x, key, cmp);
    if (i != -1)
    {
        x.Remove(i);
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------

template<typename T>
inline bool FindAdd(Array<T>& x, const T& key, const Equatable<T> cmp = OpEquatable<T>())
{
    i32 i = RFind(x, key, cmp);
    if (i != -1)
    {
        x.Grow() = key;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 FindMax(const T* const ptr, i32 count, const Comparable<T> cmp = OpComparable<T>())
{
    if (count <= 0)
    {
        return -1;
    }
    i32 j = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (cmp.Compare(ptr[i] ptr[j]) > 0)
        {
            j = i;
        }
    }
    return j;
}
template<typename T>
inline i32 FindMax(const Slice<const T> x, const Comparable<T> cmp = OpComparable<T>())
{
    return FindMax(x.begin(), x.Size(), cmp);
}
template<typename T>
inline i32 FindMax(const Array<T> x, const Comparable<T> cmp = OpComparable<T>())
{
    return FindMax(x.begin(), x.Size(), cmp);
}

// ----------------------------------------------------------------------------

template<typename T>
inline constexpr i32 FindMin(const T* const ptr, i32 count, const Comparable<T> cmp = OpComparable<T>())
{
    if (count <= 0)
    {
        return -1;
    }
    i32 j = 0;
    for (i32 i = 1; i < count; ++i)
    {
        if (cmp.Compare(ptr[i], ptr[j]) < 0)
        {
            j = i;
        }
    }
    return j;
}
template<typename T>
inline i32 FindMin(const Slice<const T> x, const Comparable<T> cmp = OpComparable<T>())
{
    return FindMin(x.begin(), x.Size(), cmp);
}
template<typename T>
inline i32 FindMin(const Array<T> x, const Comparable<T> cmp = OpComparable<T>())
{
    return FindMin(x.begin(), x.Size(), cmp);
}

// ----------------------------------------------------------------------------
