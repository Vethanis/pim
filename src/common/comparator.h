#pragma once

#include "common/hash.h"
#include <string.h>

template<typename T>
struct Comparable
{
    using CompareFn = i32(*)(const T& lhs, const T& rhs);
    const CompareFn Compare;

    inline i32 operator()(const T& lhs, const T& rhs) const
    {
        return Compare(lhs, rhs);
    }
};

template<typename T>
struct Equatable
{
    using EqualsFn = bool(*)(const T& lhs, const T& rhs);
    const EqualsFn Equals;

    inline bool operator()(const T& lhs, const T& rhs) const
    {
        return Equals(lhs, rhs);
    }
};

template<typename T>
struct Hashable
{
    using HashFn = u32(*)(const T& item);
    const HashFn Hash;

    inline u32 operator()(const T& item) const
    {
        return Hash(item);
    }
};

template<typename T>
struct Comparator
{
    const Equatable<T> Equals;
    const Comparable<T> Compare;
    const Hashable<T> Hash;
};
