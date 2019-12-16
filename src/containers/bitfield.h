#pragma once

#include "common/round.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/initlist.h"

template<typename T, u32 ct>
struct BitField
{
    static constexpr u32 NumBits = ct;
    static constexpr u32 NumDwords = DivRound(NumBits, 32u);

    u32 m_dwords[NumDwords];

    inline constexpr BitField() : m_dwords() {}

    inline constexpr BitField(std::initializer_list<T> list) : m_dwords()
    {
        for (T x : list)
        {
            u32 i = x >> 5;
            u32 j = x & 31;
            u32 mask = 1u << j;
            m_dwords[i] |= mask;
        }
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

template<typename T, u32 ct>
inline BitField<T, ct>& operator ~(BitField<T, ct>& x)
{
    constexpr u32 count = x.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        x.m_dwords[i] = ~x.m_dwords[i];
    }
    return x;
}

template<typename T, u32 ct>
inline BitField<T, ct> operator | (BitField<T, ct> lhs, BitField<T, ct> rhs)
{
    BitField<T, ct> result;
    constexpr u32 count = lhs.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] | rhs.m_dwords[i];
    }
    return result;
}

template<typename T, u32 ct>
inline BitField<T, ct> operator & (BitField<T, ct> lhs, BitField<T, ct> rhs)
{
    BitField<T, ct> result;
    constexpr u32 count = lhs.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] & rhs.m_dwords[i];
    }
    return result;
}

template<typename T, u32 ct>
inline BitField<T, ct> operator ^ (BitField<T, ct> lhs, BitField<T, ct> rhs)
{
    BitField<T, ct> result;
    constexpr u32 count = lhs.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        result.m_dwords[i] = lhs.m_dwords[i] ^ rhs.m_dwords[i];
    }
    return result;
}

template<typename T, u32 ct>
inline bool operator == (BitField<T, ct> lhs, BitField<T, ct> rhs)
{
    constexpr u32 count = lhs.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        if (lhs.m_dwords[i] != rhs.m_dwords[i])
        {
            return false;
        }
    }
    return true;
}

template<typename T, u32 ct>
inline bool operator != (BitField<T, ct> lhs, BitField<T, ct> rhs)
{
    constexpr u32 count = lhs.NumDwords;
    for (u32 i = 0; i < count; ++i)
    {
        if (lhs.m_dwords[i] == rhs.m_dwords[i])
        {
            return false;
        }
    }
    return true;
}
