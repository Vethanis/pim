#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct prng_s prng_t;

typedef struct guid_s
{
    u64 a;
    u64 b;
} guid_t;

bool guid_isnull(guid_t x);
bool guid_eq(guid_t lhs, guid_t rhs);
i32 guid_cmp(guid_t lhs, guid_t rhs);

guid_t guid_new(void);

void guid_set_name(guid_t id, const char* str);
bool guid_get_name(guid_t id, char* dst, i32 size);

i32 guid_find(guid_t const *const pim_noalias ptr, i32 count, guid_t key);
guid_t guid_str(char const *const pim_noalias str);
guid_t guid_bytes(void const *const pim_noalias ptr, i32 nBytes);
guid_t guid_rand(prng_t* rng);

void guid_fmt(char* dst, i32 size, guid_t value);
u32 guid_hashof(guid_t x);

PIM_C_END
