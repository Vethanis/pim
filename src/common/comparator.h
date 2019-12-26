#pragma once

#include "common/hash.h"
#include <string.h>

template<typename T>
struct Comparator
{
    using EqualsFn = bool(*)(const T& lhs, const T& rhs);
    using CompareFn = i32(*)(const T& lhs, const T& rhs);
    using HashFn = u32(*)(const T& item);

    const EqualsFn Equals;
    const CompareFn Compare;
    const HashFn Hash;
};

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

template<typename T>
static bool MemEqualsFn(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) == 0;
}

template<typename T>
static i32 MemCompareFn(const T& lhs, const T& rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T));
}

template<typename T>
static u32 MemHashFn(const T& item)
{
    return Fnv32Bytes(&item, sizeof(T));
}

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
static constexpr Comparator<T> MemComparator()
{
    return Comparator<T>
    {
        MemEqualsFn<T>,
        MemCompareFn<T>,
        MemHashFn<T>,
    };
}
