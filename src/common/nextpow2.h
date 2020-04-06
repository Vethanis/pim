#pragma once

#include "common/macro.h"

PIM_C_BEGIN

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static u32 NextPow2(u32 x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return x;
}

static bool IsPow2(u32 x)
{
    return (x & (x - 1u)) == 0u;
}

PIM_C_END
