#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct guid_s
{
    uint64_t a;
    uint64_t b;
} guid_t;

static int32_t guid_isnull(guid_t x)
{
    return !(x.a | x.b);
}

static int32_t guid_eq(guid_t lhs, guid_t rhs)
{
    return !((lhs.a - rhs.a) | (lhs.b - rhs.b));
}

int32_t guid_find(const guid_t* ptr, int32_t count, guid_t key);
guid_t StrToGuid(const char* str, uint64_t seed);
guid_t BytesToGuid(const void* ptr, int32_t nBytes, uint64_t seed);
guid_t RandGuid();

PIM_C_END
