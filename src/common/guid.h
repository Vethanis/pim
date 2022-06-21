#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Guid_s
{
    pim_alignas(16) u64 a;
    u64 b;
} Guid;

bool VEC_CALL Guid_IsNull(Guid x);
bool VEC_CALL Guid_Equal(Guid lhs, Guid rhs);
i32 VEC_CALL Guid_Compare(Guid lhs, Guid rhs);

Guid VEC_CALL Guid_New(void);

void VEC_CALL Guid_SetName(Guid id, const char* str);
bool VEC_CALL Guid_GetName(Guid id, char* dst, i32 size);

i32 VEC_CALL Guid_Find(Guid const *const pim_noalias ptr, i32 count, Guid key);
// adds str to a guid->string lookup table
Guid VEC_CALL Guid_FromStr(char const *const pim_noalias str);
// only hashes str
Guid VEC_CALL Guid_HashStr(char const *const pim_noalias str);
Guid VEC_CALL Guid_FromBytes(void const *const pim_noalias ptr, i32 nBytes);

void VEC_CALL Guid_Format(char* dst, i32 size, Guid value);
u32 VEC_CALL Guid_HashOf(Guid x);

PIM_C_END
