#pragma once

#include "common/int_types.h"
#include "common/hash.h"

struct Guid
{
    u32 a, b, c, d;

    inline constexpr Guid()
        : a(0), b(0), c(0), d(0)
    {}
    inline constexpr Guid(cstrc str)
        : a(0), b(0), c(0), d(0)
    {
        i32 len = 0;
        while (str[len])
        {
            ++len;
        }
        a = Fnv32Bytes(str, len);
        a = a ? a : 1;
        b = Fnv32Bytes(str, len, a);
        b = b ? b : 1;
        c = Fnv32Bytes(str, len, b);
        c = c ? c : 1;
        d = Fnv32Bytes(str, len, c);
        d = d ? d : 1;
    }
    inline constexpr Guid(const void* const ptr, i32 count)
        : a(0), b(0), c(0), d(0)
    {
        a = Fnv32Bytes(ptr, count);
        a = a ? a : 1;
        b = Fnv32Bytes(ptr, count, a);
        b = b ? b : 1;
        c = Fnv32Bytes(ptr, count, b);
        c = c ? c : 1;
        d = Fnv32Bytes(ptr, count, c);
        d = d ? d : 1;
    }
    inline constexpr bool IsNull() const
    {
        return !(a | b | c | d);
    }
    inline constexpr bool operator==(Guid o) const
    {
        u32 cmp = 0;
        cmp |= a - o.a;
        cmp |= b - o.b;
        cmp |= c - o.c;
        cmp |= d - o.d;
        return cmp == 0;
    }
};

struct GuidStr
{
    // 0x
    // 32 nibbles
    // null terminator
    static constexpr i32 Length = 2 + 16 * 2;
    char Value[Length + 1];
    inline operator cstr() const { return Value; }
};

namespace Guids
{
    Guid FromString(GuidStr str);
    GuidStr ToString(Guid guid);
};
