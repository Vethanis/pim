#pragma once

#include "common/int_types.h"
#include "common/hash.h"

struct Guid
{
    u32 a, b, c, d;

    inline constexpr bool operator==(Guid o) const
    {
        u32 cmp = 0;
        cmp |= a - o.a;
        cmp |= b - o.b;
        cmp |= c - o.c;
        cmp |= d - o.d;
        return cmp == 0;
    }
    inline constexpr bool operator<(Guid o) const
    {
        if (a != o.a)
        {
            return a < o.a;
        }
        if (b != o.b)
        {
            return b < o.b;
        }
        if (c != o.c)
        {
            return c < o.c;
        }
        if (d != o.d)
        {
            return d < o.d;
        }
        return false;
    }
};

inline constexpr bool IsNull(Guid g)
{
    return !(g.a | g.b | g.c | g.d);
}

inline constexpr Guid ToGuid(cstrc str)
{
    u32 a = Fnv32String(str);
    a = a ? a : 1;
    u32 b = Fnv32String(str, a);
    b = b ? b : 1;
    u32 c = Fnv32String(str, b);
    c = c ? c : 1;
    u32 d = Fnv32String(str, c);
    d = d ? d : 1;

    return Guid { a, b, c, d };
}

inline Guid ToGuid(const void* ptr, i32 count)
{
    u32 a = Fnv32Bytes(ptr, count);
    a = a ? a : 1;
    u32 b = Fnv32Bytes(ptr, count, a);
    b = b ? b : 1;
    u32 c = Fnv32Bytes(ptr, count, b);
    c = c ? c : 1;
    u32 d = Fnv32Bytes(ptr, count, c);
    d = d ? d : 1;

    return Guid { a, b, c, d };
}
