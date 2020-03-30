#pragma once

#include <stdint.h>

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static uint32_t NextPow2(uint32_t x)
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
