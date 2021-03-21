#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Prng_s Prng;

typedef struct Guid_s
{
    u64 a;
    u64 b;
} Guid;

bool guid_isnull(Guid x);
bool guid_eq(Guid lhs, Guid rhs);
i32 guid_cmp(Guid lhs, Guid rhs);

Guid guid_new(void);

void guid_set_name(Guid id, const char* str);
bool guid_get_name(Guid id, char* dst, i32 size);

i32 guid_find(Guid const *const pim_noalias ptr, i32 count, Guid key);
Guid guid_str(char const *const pim_noalias str);
Guid guid_bytes(void const *const pim_noalias ptr, i32 nBytes);
Guid guid_rand(Prng* rng);

void guid_fmt(char* dst, i32 size, Guid value);
u32 guid_hashof(Guid x);

PIM_C_END
