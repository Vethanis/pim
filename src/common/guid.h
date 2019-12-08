#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"

struct Guid
{
    u32 a, b, c, d;

    inline bool IsNull() const
    {
        return !(a | b | c | d);
    }
};

struct GuidStr
{
    // 0x
    // 32 nibbles
    // null terminator
    char Value[2 + 16 * 2 + 1];

    inline operator cstr() const { return Value; }
};

namespace Guids
{
    inline constexpr Guid AsHash(cstrc str)
    {
        if (str)
        {
            u32 a = Fnv32String(str);
            a = a ? a : 1;
            u32 b = Fnv32String(str, a);
            b = b ? b : 1;
            u32 c = Fnv32String(str, b);
            c = c ? c : 1;
            u32 d = Fnv32String(str, c);
            d = d ? d : 1;
            return { a, b, c, d };
        }
        return { 0, 0, 0, 0 };
    }

    Guid FromString(GuidStr str);
    GuidStr ToString(Guid guid);
};
