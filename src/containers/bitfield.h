#pragma once

#include "containers/slice.h"
#include "containers/array.h"

template<u32 ct>
struct BitField
{
    static constexpr u32 NumBits = ct;
    static constexpr u32 NumDwords = DivRound(NumBits, 32);

    u32 m_dwords[NumDwords];

    inline BitField() : m_dwords()
    {
        for (u32& x : m_dwords)
            x = 0;
    }

    inline void Clear()
    {
        for (u32& x : m_dwords)
            x = 0;
    }
    inline void Set(u32 bit)
    {
        u32 i = bit >> 5;
        u32 j = bit & 31;
        u32 mask = 1u << j;
        m_dwords[i] |= mask;
    }
    inline void UnSet(u32 bit)
    {
        u32 i = bit >> 5;
        u32 j = bit & 31;
        u32 mask = 1u << j;
        m_dwords[i] &= ~mask;
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
        return (m_dwords[i] & mask) != 0;
    }
    inline void FromEnum(Slice<const u32> enums)
    {
        for (u32 value : enums)
        {
            Set(value);
        }
    }
    inline void ToEnum(Array<u32>& enums) const
    {
        enums.clear();
        for (u32 i = 0; i < NumBits; ++i)
        {
            if (Has(i))
            {
                enums.grow() = i;
            }
        }
    }
    inline bool Any() const
    {
        for (u32 x : m_dwords)
        {
            if (x != 0u)
            {
                return true;
            }
        }
        return false;
    }
    inline bool None() const
    {
        return !Any();
    }
};

template<u32 ct>
inline BitField<ct>& operator ~(BitField<ct>& x)
{
    for (u32& dword : x.m_dwords)
    {
        dword = ~dword;
    }
    return x;
}

template<u32 ct>
inline BitField<ct> operator | (BitField<ct> lhs, BitField<ct> rhs)
{
    BitField<ct> result;
    for (u32 i = 0; i < lhs.NumDwords; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] | rhs.m_dwords[i];
    }
    return result;
}

template<u32 ct>
inline BitField<ct> operator & (BitField<ct> lhs, BitField<ct> rhs)
{
    BitField<ct> result;
    for (u32 i = 0; i < lhs.NumDwords; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] & rhs.m_dwords[i];
    }
    return result;
}

template<u32 ct>
inline BitField<ct> operator ^ (BitField<ct> lhs, BitField<ct> rhs)
{
    BitField<ct> result;
    for (u32 i = 0; i < lhs.NumDwords; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] ^ rhs.m_dwords[i];
    }
    return result;
}

template<u32 ct>
inline bool operator == (BitField<ct> lhs, BitField<ct> rhs)
{
    for (u32 i = 0; i < lhs.NumDwords; ++i)
    {
        if (lhs.m_dwords[i] != rhs.m_dwords[i])
        {
            return false;
        }
    }
    return true;
}

template<u32 ct>
inline bool operator != (BitField<ct> lhs, BitField<ct> rhs)
{
    for (u32 i = 0; i < lhs.NumDwords; ++i)
    {
        if (lhs.m_dwords[i] == rhs.m_dwords[i])
        {
            return false;
        }
    }
    return true;
}
