#pragma once

#include "common/hash.h"
#include <string.h>

template<typename T>
struct Comparator
{
    using EqualsFn = bool(*)(T lhs, T rhs);
    using CompareFn = i32(*)(T lhs, T rhs);
    using HashFn = u32(*)(T item);

    const EqualsFn Equals;
    const CompareFn Compare;
    const HashFn Hash;
};

template<typename T>
static bool OpEqualsFn(T lhs, T rhs)
{
    return lhs == rhs;
}

template<typename T>
static i32 OpCompareFn(T lhs, T rhs)
{
    if (!(lhs == rhs))
    {
        return lhs < rhs ? -1 : 1;
    }
    return 0;
}

template<typename T>
static bool MemEqualsFn(T lhs, T rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T)) == 0;
}

template<typename T>
static i32 MemCompareFn(T lhs, T rhs)
{
    return memcmp(&lhs, &rhs, sizeof(T));
}

template<typename T>
static u32 MemHashFn(T item)
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
