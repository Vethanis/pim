#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#define Fnv32Bias 2166136261u
#define Fnv64Bias 14695981039346656037ull

char FnvToUpper(char x);

u32 Fnv32Char(char x, u32 hash);
u32 Fnv32Byte(u8 x, u32 hash);
u32 Fnv32Word(u16 x, u32 hash);
u32 Fnv32Dword(u32 x, u32 hash);
u32 Fnv32Qword(u64 x, u32 hash);
u32 Fnv32String(char const *const pim_noalias str, u32 hash);
u32 Fnv32Bytes(void const *const pim_noalias ptr, i32 nBytes, u32 hash);

u64 Fnv64Char(char x, u64 hash);
u64 Fnv64Byte(u8 x, u64 hash);
u64 Fnv64Word(u16 x, u64 hash);
u64 Fnv64Dword(u32 x, u64 hash);
u64 Fnv64Qword(u64 x, u64 hash);
u64 Fnv64String(char const *const pim_noalias ptr, u64 hash);
u64 Fnv64Bytes(void const *const pim_noalias ptr, i32 nBytes, u64 hash);

u32 HashStr(char const *const pim_noalias text);
u32 HashBytes(void const *const pim_noalias ptr, i32 nBytes);

PIM_C_END
