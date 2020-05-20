#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct sdict_s
{
    u32* hashes;
    char** keys;
    void* values;
    u32 width;
    u32 count;
    u32 valueSize;
    EAlloc allocator;
} sdict_t;

void sdict_new(sdict_t* dict, u32 valueSize, EAlloc allocator);
void sdict_del(sdict_t* dict);

void sdict_clear(sdict_t* dict);
void sdict_reserve(sdict_t* dict, i32 count);

i32 sdict_find(const sdict_t* dict, const char* key);
bool sdict_get(const sdict_t* dict, const char* key, void* valueOut);
bool sdict_set(sdict_t* dict, const char* key, const void* value);
bool sdict_add(sdict_t* dict, const char* key, const void* value);
bool sdict_rm(sdict_t* dict, const char* key, void* valueOut);

typedef i32(*SDictCmpFn)(
    const char* lkey, const char* rkey,
    const void* lval, const void* rval,
    void* usr);

u32* sdict_sort(const sdict_t* dict, SDictCmpFn cmp, void* usr);

// StrCmp sort metric
i32 SDictStrCmp(
    const char* lKey, const char* rKey,
    const void* lVal, const void* rVal,
    void* usr);

PIM_C_END
