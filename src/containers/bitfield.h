#pragma once

#include "common/int_types.h"
#include "common/round.h"
#include "os/atomics.h"
#include <initializer_list>

template<typename T, u32 ct>
struct BitField
{
    static constexpr u32 NumBits = ct;
    static constexpr u32 NumDwords = DivRound(NumBits, 32u);

    SASSERT(sizeof(T) == sizeof(u32));

    u32 Values[NumDwords];

    constexpr BitField() : Values() {}

    constexpr BitField(std::initializer_list<T> list) : Values()
    {
        for (T x : list)
        {
            u32 value = *reinterpret_cast<u32*>(&x);
            Set(value);
        }
    }

    static u32 IndexOf(u32 bit)
    {
        u32 i = bit >> 5;
        ASSERT(i < NumDwords);
        return i;
    }

    static u32 MaskOf(u32 bit)
    {
        return 1u << (bit & 31u);
    }

    void Clear()
    {
        for (u32& x : Values)
            Store(x, 0);
    }

    bool Set(u32 bit)
    {
        u32 i = IndexOf(bit);
        u32 mask = MaskOf(bit);
        return FetchOr(Values[i], mask) & mask;
    }

    bool UnSet(u32 bit)
    {
        u32 i = IndexOf(bit);
        u32 mask = MaskOf(bit);
        return FetchAnd(Values[i], ~mask) & mask;
    }

    bool Toggle(u32 bit)
    {
        u32 i = IndexOf(bit);
        u32 mask = MaskOf(bit);
        return FetchXor(Values[i], mask) & mask;
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

    bool Get(u32 bit) const
    {
        u32 i = IndexOf(bit);
        u32 mask = MaskOf(bit);
        return Load(Values[i], MO_Relaxed) & mask;
    }

    bool Any() const
    {
        u32 y = 0u;
        for (u32 i = 0; i < NumDwords; ++i)
        {
            y |= Load(Values[i], MO_Relaxed);
        }
        return y != 0u;
    }

    bool None() const
    {
        return !Any();
    }

    static bool HasAll(const BitField& all, const BitField& x)
    {
        for (u32 i = 0; i < NumDwords; ++i)
        {
            u32 l = Load(all.Values[i], MO_Relaxed);
            u32 r = Load(x.Values[i], MO_Relaxed);
            if ((l & r) != l)
            {
                return false;
            }
        }
        return true;
    }

    static bool HasNone(const BitField& none, const BitField& x)
    {
        for (u32 i = 0; i < NumDwords; ++i)
        {
            u32 l = Load(none.Values[i], MO_Relaxed);
            u32 r = Load(x.Values[i], MO_Relaxed);
            if ((l & r) != 0)
            {
                return false;
            }
        }
        return true;
    }

    BitField operator ~() const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 dword = Load(Values[i], MO_Relaxed);
            result.Values[i] = ~dword;
        }
        return result;
    }

    BitField operator | (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            result.Values[i] = lhs | rhs;
        }
        return result;
    }

    BitField operator & (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            result.Values[i] = lhs & rhs;
        }
        return result;
    }

    BitField operator ^ (const BitField& r) const
    {
        BitField result = {};
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            result.Values[i] = lhs ^ rhs;
        }
        return result;
    }

    bool operator == (const BitField& r) const
    {
        u32 cmp = 0;
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            cmp |= lhs - rhs;
        }
        return cmp == 0;
    }

    bool operator != (const BitField& r) const
    {
        u32 cmp = 0;
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            cmp |= lhs - rhs;
        }
        return cmp != 0;
    }

    bool operator < (const BitField& r) const
    {
        for (i32 i = 0; i < NumDwords; ++i)
        {
            u32 lhs = Load(Values[i], MO_Relaxed);
            u32 rhs = Load(r.Values[i], MO_Relaxed);
            if (lhs != rhs)
            {
                return lhs < rhs;
            }
        }
        return false;
    }
};
