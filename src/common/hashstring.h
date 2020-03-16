#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

static uint32_t HashStr(const char* text)
{
    const uint32_t kBias = 2166136261u;
    const uint32_t kPrime = 16777619u;
    uint32_t value = 0;
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

static int32_t HashFind(
    const uint32_t* const hashes,
    int32_t count,
    uint32_t hash)
{
    ASSERT(hashes);
    ASSERT(count >= 0);
    ASSERT(hash != 0);
    for (int32_t i = 0; i < count; ++i)
    {
        if (hashes[i] == hash)
        {
            return i;
        }
    }
    return -1;
}

PIM_C_END
