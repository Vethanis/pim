#pragma once

#include "common/macro.h"

PIM_C_BEGIN

static u32 HashStr(const char* text)
{
    const u32 kBias = 2166136261u;
    const u32 kPrime = 16777619u;
    u32 value = 0;
    if (text)
    {
        value = kBias;
        while (*text)
        {
            value = (value ^ *text++) * kPrime;
        }
        value = value ? value : 1;
    }
    return value;
}

static i32 HashFind(const u32* const hashes, i32 count, u32 hash)
{
    ASSERT(count >= 0);
    ASSERT(hash != 0);
    for (i32 i = 0; i < count; ++i)
    {
        if (hashes[i] == hash)
        {
            return i;
        }
    }
    return -1;
}

PIM_C_END
