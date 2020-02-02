#pragma once

#include "common/comparator.h"

// ----------------------------------------------------------------------------

template<typename T>
static bool OpEqualsFn(const T& lhs, const T& rhs)
{
    return lhs == rhs;
}

template<typename T>
static i32 OpCompareFn(const T& lhs, const T& rhs)
{
    if (!(lhs == rhs))
    {
        return lhs < rhs ? -1 : 1;
    }
    return 0;
}

// ----------------------------------------------------------------------------

template<typename T>
static i32 MemCompareFn(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T));
}

template<typename T>
static bool MemEqualsFn(const T& lhs, const T& rhs)
{
    return MemCompareFn(lhs, rhs) == 0;
}

template<typename T>
static u32 MemHashFn(const T& item)
{
    return Fnv32Bytes(&item, sizeof(T));
}

// ----------------------------------------------------------------------------

template<typename T>
static i32 PtrCompareFn(const T& lhs, const T& rhs)
{
    return (i32)(lhs - rhs);
}

template<typename T>
static bool PtrEqualsFn(const T& lhs, const T& rhs)
{
    return lhs == rhs;
}

template<typename T>
static u32 PtrHashFn(const T& item)
{
    return Fnv32Qword((u64)item);
}

// ----------------------------------------------------------------------------

template<typename T>
static constexpr Comparator<T> OpComparator()
{
    return Comparator<T>
    {
        OpEqualsFn<T>,
            OpCompareFn<T>,
            MemHashFn<T>,
    };
}

template<typename T>
static constexpr Comparable<T> OpComparable()
{
    return Comparable<T>
    {
        OpCompareFn<T>,
    };
}

template<typename T>
static constexpr Equatable<T> OpEquatable()
{
    return Equatable<T>
    {
        OpEqualsFn<T>,
    };
}

// ----------------------------------------------------------------------------

template<typename T>
static constexpr Comparator<T> MemComparator()
{
    return Comparator<T>
    {
        MemEqualsFn<T>,
            MemCompareFn<T>,
            MemHashFn<T>,
    };
}

template<typename T>
static constexpr Comparable<T> MemComparable()
{
    return Comparable<T>
    {
        MemCompareFn<T>,
    };
}

template<typename T>
static constexpr Equatable<T> MemEquatable()
{
    return Equatable<T>
    {
        MemEqualsFn<T>,
    };
}

template<typename T>
static constexpr Hashable<T> MemHashable()
{
    return Hashable<T>
    {
        MemHashFn<T>,
    };
}

// ----------------------------------------------------------------------------

template<typename T>
static constexpr Comparator<T> PtrComparator()
{
    return Comparator<T>
    {
        PtrEqualsFn<T>,
        PtrCompareFn<T>,
        PtrHashFn<T>,
    };
}

template<typename T>
static constexpr Comparable<T> PtrComparable()
{
    return Comparable<T>
    {
        PtrCompareFn<T>,
    };
}

template<typename T>
static constexpr Equatable<T> PtrEquatable()
{
    return Equatable<T>
    {
        PtrEqualsFn<T>,
    };
}

template<typename T>
static constexpr Hashable<T> PtrHashable()
{
    return Hashable<T>
    {
        PtrHashFn<T>,
    };
}

// ----------------------------------------------------------------------------
