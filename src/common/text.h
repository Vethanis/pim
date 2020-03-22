#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<i32 t_capacity>
struct Text
{
    static constexpr i32 Capacity = t_capacity;

    char value[Capacity];

    Text() { for (char& x : value) { x = 0; } }

    Text(cstrc src) : value()
    {
        for (char& x : value) { x = 0; }
        if (src)
        {
            for (i32 i = 0; i < Capacity; ++i)
            {
                if (src[i])
                {
                    value[i] = src[i];
                }
                else
                {
                    break;
                }
            }
            value[Capacity - 1] = 0;
        }
    }

    i32 capacity() const { return Capacity; }

    i32 size() const
    {
        i32 len = 0;
        for (; len < Capacity; ++len)
        {
            if (!value[len])
            {
                break;
            }
        }
        return len;
    }

    cstr begin() const { return value; }
    cstr end() const { return value + size(); }

    char* begin() { return value; }
    char* end() { return value + size(); }

    operator cstr () const { return value; }

    bool operator==(const Text& rhs) const
    {
        for (i32 i = 0; i < Capacity; ++i)
        {
            if (value[i] != rhs.value[i])
            {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Text& rhs) const
    {
        for (i32 i = 0; i < Capacity; ++i)
        {
            if (value[i] != rhs.value[i])
            {
                return true;
            }
        }
        return false;
    }

    bool operator<(const Text& rhs) const
    {
        for (i32 i = 0; i < Capacity; ++i)
        {
            i32 cmp = value[i] - rhs.value[i];
            if (cmp)
            {
                return cmp < 0;
            }
        }
        return false;
    }
};

using PathText = Text<PIM_PATH>;
