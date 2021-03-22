#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct StrDict_s
{
    u32* hashes;
    char** keys;
    void* values;
    u32 width;
    u32 count;
    u32 valueSize;
    EAlloc allocator;
} StrDict;

void StrDict_New(StrDict* dict, u32 valueSize, EAlloc allocator);
void StrDict_Del(StrDict* dict);

void StrDict_Clear(StrDict* dict);
void StrDict_Reserve(StrDict* dict, i32 count);

i32 StrDict_Find(const StrDict* dict, const char* key);
bool StrDict_Get(const StrDict* dict, const char* key, void* valueOut);
bool StrDict_Set(StrDict* dict, const char* key, const void* value);
bool StrDict_Add(StrDict* dict, const char* key, const void* value);
bool StrDict_Rm(StrDict* dict, const char* key, void* valueOut);

typedef i32(*SDictCmpFn)(
    const char* lkey, const char* rkey,
    const void* lval, const void* rval,
    void* usr);

u32* StrDict_Sort(const StrDict* dict, SDictCmpFn cmp, void* usr);

// StrCmp sort metric
i32 SDictStrCmp(
    const char* lKey, const char* rKey,
    const void* lVal, const void* rVal,
    void* usr);

PIM_C_END
