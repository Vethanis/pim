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

static bool guid_isnull(guid_t x)
{
    return !(x.a | x.b);
}

static bool guid_eq(guid_t lhs, guid_t rhs)
{
    return !((lhs.a - rhs.a) | (lhs.b - rhs.b));
}

i32 guid_find(const guid_t* ptr, i32 count, guid_t key);
guid_t guid_str(const char* str, u64 seed);
guid_t guid_bytes(const void* ptr, i32 nBytes, u64 seed);
guid_t guid_rand(prng_t* rng);

PIM_C_END
