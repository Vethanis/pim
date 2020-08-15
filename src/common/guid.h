#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#define guid_seed 14695981039346656037ull

typedef struct prng_s prng_t;

typedef struct guid_s
{
    u64 a;
    u64 b;
} guid_t;

pim_inline bool guid_isnull(guid_t x)
{
    return !(x.a | x.b);
}

pim_inline bool guid_eq(guid_t lhs, guid_t rhs)
{
    return !((lhs.a - rhs.a) | (lhs.b - rhs.b));
}

pim_inline i32 guid_cmp(guid_t lhs, guid_t rhs)
{
    if (lhs.a != rhs.a)
    {
        return lhs.a < rhs.a ? -1 : 1;
    }
    if (lhs.b != rhs.b)
    {
        return lhs.b < rhs.b ? -1 : 1;
    }
    return 0;
}

i32 guid_find(const guid_t* pim_noalias ptr, i32 count, guid_t key);
guid_t guid_str(const char* str, u64 seed);
guid_t guid_bytes(const void* ptr, i32 nBytes, u64 seed);
guid_t guid_rand(prng_t* rng);

void guid_fmt(char* dst, i32 size, guid_t value);
u32 guid_hashof(guid_t x);

PIM_C_END
