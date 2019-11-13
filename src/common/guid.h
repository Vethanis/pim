#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/hash.h"

struct Guid
{
    u64 Value[2];

    inline bool operator==(const Guid& o) const
    {
        return !((Value[0] - o.Value[0]) | (Value[1] - o.Value[1]));
    }

    inline bool IsNull() const
    {
        return !(Value[0] | Value[1]);
    }
};

struct GuidStr
{
    char Value[2 + 16 * 2];
};

namespace Guids
{
    inline constexpr Guid AsHash(cstrc str)
    {
        if (str)
        {
            u64 q0 = Fnv64String(str);
            u64 q1 = Fnv64String(str, q0);
            q0 = q0 ? q0 : 1;
            q1 = q1 ? q1 : 1;
            return { q0, q1 };
        }
        return { 0, 0 };
    }

    Guid FromString(GuidStr str);
    GuidStr ToString(Guid guid);
};
