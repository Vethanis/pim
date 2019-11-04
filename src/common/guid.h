#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/random.h"
#include "common/hash.h"
#include "containers/slice.h"

struct Guid
{
    const u64 Value[2];

    static Guid Create(Slice<const u8> bytes)
    {
        i32 half = bytes.size() >> 1;
        if (half > 0)
        {
            return
            {
                Fnv64(bytes.head(half)),
                Fnv64(bytes.tail(half))
            };
        }
        return { 0, 0 };
    }

    static Guid Create(cstrc str)
    {
        if (str)
        {
            i32 len = 0;
            for (; str[len]; ++len) {}
            return Create({ (const u8*)str, len });
        }
        return { 0, 0 };
    }

    static Guid Create()
    {
        return { Random::NextU64(), Random::NextU64() };
    }
};
