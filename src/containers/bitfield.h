#pragma once

#include "common/round.h"
#include "containers/initlist.h"

template<typename T, u32 ct>
struct BitField
{
    static constexpr u32 NumBits = ct;
    static constexpr u32 NumDwords = DivRound(NumBits, 32u);

    u32 Values[NumDwords];

    inline constexpr BitField() : Values() {}

    inline constexpr BitField(std::initializer_list<T> list) : Values()
    {
        for (T x : list)
        {
            u32 i = x >> 5;
            u32 j = x & 31;
            u32 mask = 1u << j;
            Values[i] |= mask;
        }
    }

    inline void Clear()
    {
        for (u32& x : Values)
            x = 0;
    }
    inline void Set(u32 bit)
    {
        u32 i = bit >> 5;
        u32 j = bit & 31;
        u32 mask = 1u << j;
        Values[i] |= mask;
    }
    inline void UnSet(u32 bit)
    {
        u32 i = bit >> 5;
        u32 j = bit & 31;
        u32 mask = 1u << j;
        Values[i] &= ~mask;
    }
    inline void Set(u32 bit, bool on)
    {
        if (on)
        {
            Set(bit);
        }
        else
        {
            UnSet(bit);
        }
    }
    inline bool Has(u32 bit) const
    {
        u32 i = bit >> 5;
        u32 j = bit & 31;
        u32 mask = 1u << j;
        return (Values[i] & mask) != 0;
    }
    inline bool Any() const
    {
        u32 y = 0u;
        for (u32 x : Values)
        {
            y |= x;
        }
        return y != 0u;
    }
    inline bool None() const
    {
        return !Any();
    }

    inline BitField operator ~() const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = ~Values[i];
        }
        return result;
    }

    inline BitField operator | (BitField r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] | r.Values[i];
        }
        return result;
    }

    inline BitField operator & (BitField r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] & r.Values[i];
        }
        return result;
    }

    inline BitField operator ^ (BitField r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] ^ r.Values[i];
        }
        return result;
    }

    inline bool operator == (BitField r) const
    {
        u32 cmp = 0;
        for (i32 i = 0; i < NumDwords; ++i)
        {
            cmp |= Values[i] - r.Values[i];
        }
        return cmp == 0;
    }

    inline bool operator != (BitField r) const
    {
        BitField l = *this;
        return !(l == r);
    }

    inline bool operator < (BitField r) const
    {
        for (i32 i = 0; i < NumDwords; ++i)
        {
            if (Values[i] != r.Values[i])
            {
                return Values[i] < r.Values[i];
            }
        }
        return false;
    }
};
