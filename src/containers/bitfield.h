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
            Set(x);
        }
    }

    inline static u32 IndexOf(u32 bit)
    {
        u32 i = bit >> 5;
        ASSERT(i < NumDwords);
        return i;
    }
    inline static u32 MaskOf(u32 bit)
    {
        return 1u << (bit & 31u);
    }

    inline void Clear()
    {
        for (u32& x : Values)
            x = 0;
    }

    inline void Set(u32 bit)
    {
        Values[IndexOf(bit)] |= MaskOf(bit);
    }
    inline void UnSet(u32 bit)
    {
        Values[IndexOf(bit)] &= ~MaskOf(bit);
    }
    inline void Toggle(u32 bit)
    {
        Values[IndexOf(bit)] ^= MaskOf(bit);
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
    inline bool Get(u32 bit) const
    {
        return Values[IndexOf(bit)] & MaskOf(bit);
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
