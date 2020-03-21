#pragma once

#include "common/int_types.h"
#include "common/round.h"
#include <initializer_list>

template<typename T, u32 ct>
struct alignas(16) BitField
{
    static constexpr u32 NumBits = ct;
    static constexpr u32 NumDwords = DIV_ROUND(NumBits, 32u);

    SASSERT(sizeof(T) == sizeof(u32));

    u32 Values[NumDwords];

    constexpr BitField() : Values() {}

    constexpr BitField(std::initializer_list<T> list) : Values()
    {
        for (T x : list)
        {
            Set(*reinterpret_cast<u32*>(&x));
        }
    }

    static constexpr u32 IndexOf(u32 bit)
    {
        return bit >> 5u;
    }

    static constexpr u32 MaskOf(u32 bit)
    {
        return 1u << (bit & 31u);
    }

    void Clear()
    {
        for (u32& x : Values)
        {
            x = 0u;
        }
    }

    void Set(u32 bit)
    {
        Values[IndexOf(bit)] |= MaskOf(bit);
    }

    void UnSet(u32 bit)
    {
        Values[IndexOf(bit)] &= ~MaskOf(bit);
    }

    void Toggle(u32 bit)
    {
        Values[IndexOf(bit)] ^= MaskOf(bit);
    }

    void Set(u32 bit, bool on)
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

    bool Has(u32 bit) const
    {
        return Values[IndexOf(bit)] & MaskOf(bit);
    }

    bool Any() const
    {
        u32 y = 0u;
        for (u32 x : Values)
        {
            y |= x;
        }
        return y;
    }

    bool None() const
    {
        return !Any();
    }

    bool HasAll(const BitField& all) const
    {
        for (u32 i = 0; i < NumDwords; ++i)
        {
            u32 l = all.Values[i];
            u32 r = Values[i];
            if ((l & r) != l)
            {
                return false;
            }
        }
        return true;
    }

    bool HasAny(const BitField& any) const
    {
        for (u32 i = 0; i < NumDwords; ++i)
        {
            u32 l = any.Values[i];
            u32 r = Values[i];
            if ((l & r) != 0)
            {
                return true;
            }
        }
        return false;
    }

    bool HasNone(const BitField& none) const
    {
        return !HasAny(none);
    }

    BitField operator ~() const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = ~Values[i];
        }
        return result;
    }

    BitField operator | (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] | r.Values[i];
        }
        return result;
    }

    BitField operator & (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] & r.Values[i];
        }
        return result;
    }

    BitField operator ^ (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            result.Values[i] = Values[i] ^ r.Values[i];
        }
        return result;
    }

    bool operator == (const BitField& r) const
    {
        u32 cmp = 0;
        for (i32 i = 0; i < NumDwords; ++i)
        {
            cmp |= Values[i] - r.Values[i];
        }
        return cmp == 0;
    }

    bool operator != (const BitField& r) const
    {
        u32 cmp = 0;
        for (i32 i = 0; i < NumDwords; ++i)
        {
            cmp |= Values[i] - r.Values[i];
        }
        return cmp != 0;
    }

    bool operator < (const BitField& r) const
    {
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Values[i];
            u32 rhs = r.Values[i];
            if (lhs != rhs)
            {
                return lhs < rhs;
            }
        }
        return false;
    }
};
