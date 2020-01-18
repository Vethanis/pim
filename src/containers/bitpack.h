#pragma once

#include "common/int_types.h"

namespace BitPack
{
    static constexpr u32 Mask(u32 width)
    {
        return (1u << width) - 1u;
    }

    static constexpr u32 Offset(u32 i, u32 width)
    {
        return i * width;
    }

    static constexpr u32 Unmask(u32 i, u32 width)
    {
        return ~(Mask(width) << Offset(i, width));
    }

    template<u32 i, u32 width>
    static constexpr u32 Pack(u32 x)
    {
        constexpr u32 mask = Mask(width);
        constexpr u32 offset = Offset(i, width);
        ASSERT(x <= mask);
        return (x & mask) << offset;
    }

    template<u32 i, u32 width>
    static constexpr u32 Unpack(u32 x)
    {
        constexpr u32 mask = Mask(width);
        constexpr u32 offset = Offset(i, width);
        return mask & (x >> offset);
    }

    template<u32 i, u32 width>
    static constexpr u32 Zero(u32 x)
    {
        constexpr u32 unmask = Unmask(i, width);
        return x & unmask;
    }

    template<u32 count, u32 width>
    struct Value
    {
        SASSERT(count * width <= 32);

        u32 m_value;

        inline Value() : m_value(0u) {}
        inline Value(u32 value) : m_value(value) {}
        inline Value& operator=(Value value) { m_value = value.m_value; return *this; }
        inline Value& operator=(u32 value) { m_value = value; return *this; }
        inline operator u32() const { return m_value; }

        static constexpr u32 MaxValue = Mask(width);

        template<u32 i>
        inline u32 Get() const
        {
            SASSERT(i < count);
            return Unpack<i, width>(m_value);
        }

        template<u32 i>
        inline void Set(u32 x)
        {
            SASSERT(i < count);
            constexpr u32 offset = Offset(i, width);
            constexpr u32 unmask = Unmask(i, width);

            m_value = Zero<i, width>(m_value) | Pack<i, width>(x);
        }

        template<u32 i>
        inline void Add(u32 amt)
        {
            Set<i>(Get<i>() + amt);
        }

        template<u32 i>
        inline void Sub(u32 amt)
        {
            Set<i>(Get<i>() - amt);
        }

        template<u32 i>
        inline void Inc()
        {
            Add<i>(1u);
        }

        template<u32 i>
        inline void Dec()
        {
            Sub<i>(1u);
        }
    };
};
