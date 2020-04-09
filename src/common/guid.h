#pragma once

#include "common/macro.h"

PIM_C_BEGIN

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
guid_t StrToGuid(const char* str, u64 seed);
guid_t BytesToGuid(const void* ptr, i32 nBytes, u64 seed);
guid_t RandGuid(struct prng_s* rng);

PIM_C_END
