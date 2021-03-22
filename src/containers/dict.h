#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Dict_s
{
    u32 * pim_noalias hashes;
    void * pim_noalias keys;
    void * pim_noalias values;
    u32 width;
    u32 count;
    u32 keySize;
    u32 valueSize;
    EAlloc allocator;
} Dict;

void Dict_New(Dict* dict, u32 keySize, u32 valueSize, EAlloc allocator);
void Dict_Del(Dict* dict);

void Dict_Clear(Dict* dict);
void Dict_Reserve(Dict* dict, i32 count);

i32 Dict_Find(const Dict* dict, const void* key);
bool Dict_Get(const Dict* dict, const void* key, void* valueOut);
bool Dict_Set(Dict* dict, const void* key, const void* valueIn);
bool Dict_Add(Dict* dict, const void* key, const void* valueIn);
bool Dict_Rm(Dict* dict, const void* key, void* valueOut);

bool Dict_SetAdd(Dict* dict, const void* key, const void* valueIn);
bool Dict_GetAdd(Dict* dict, const void* key, void* valueOut);

typedef i32(*DictCmpFn)(
    const void* lkey, const void* rkey,
    const void* lval, const void* rval,
    void* usr);

u32* Dict_Sort(const Dict* dict, DictCmpFn cmp, void* usr);

PIM_C_END
