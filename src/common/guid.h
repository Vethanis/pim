#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Guid_s
{
    u64 a;
    u64 b;
} Guid;

bool Guid_IsNull(Guid x);
bool Guid_Equal(Guid lhs, Guid rhs);
i32 Guid_Compare(Guid lhs, Guid rhs);

Guid Guid_New(void);

void Guid_SetName(Guid id, const char* str);
bool Guid_GetName(Guid id, char* dst, i32 size);

i32 Guid_Find(Guid const *const pim_noalias ptr, i32 count, Guid key);
Guid Guid_FromStr(char const *const pim_noalias str);
Guid Guid_FromBytes(void const *const pim_noalias ptr, i32 nBytes);

void Guid_Format(char* dst, i32 size, Guid value);
u32 Guid_HashOf(Guid x);

PIM_C_END
