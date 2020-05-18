#pragma once

#include "common/macro.h"

PIM_C_BEGIN

i32 Find(const void* arr, i32 stride, i32 length, const void* key);
i32 RFind(const void* arr, i32 stride, i32 length, const void* key);
i32 StrFind(const char** arr, i32 length, const char* key);
i32 StrRFind(const char** arr, i32 length, const char* key);
i32 HashFind(const void* arr, i32 stride, const u32* hashes, i32 length, const void* key);
i32 HashRFind(const void* arr, i32 stride, const u32* hashes, i32 length, const void* key);

i32 Find_i32(const i32* values, i32 length, i32 key);
i32 Find_u32(const u32* values, i32 length, u32 key);
i32 Find_u64(const u64* values, i32 length, u64 key);
i32 Find_ptr(const void** values, i32 length, const void* key);

PIM_C_END
