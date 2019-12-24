#pragma once

#include "common/hash.h"

template<typename T>
struct IComparator
{
    virtual bool Equals(T lhs, T rhs) const = 0;
    virtual i32 Compare(T lhs, T rhs) const = 0;
    virtual u32 Hash(T item) const = 0;
};

template<typename T>
struct DefaultComparator final : IComparator<T>
{
    inline bool Equals(T lhs, T rhs) const final
    {
        return lhs == rhs;
    }
    inline i32 Compare(T lhs, T rhs) const final
    {
        if (!(lhs == rhs))
        {
            return lhs < rhs ? -1 : 1;
        }
        return 0;
    }
    inline u32 Hash(T item) const final
    {
        return Fnv32Bytes(&item, sizeof(T));
    }
};
