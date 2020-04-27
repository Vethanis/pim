#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct dict_s
{
    u32* hashes;
    void* keys;
    void* values;
    u32 count;
    u32 width;
    u32 keySize;
    u32 valueSize;
    EAlloc allocator;
} dict_t;

void dict_new(dict_t* dict, u32 keySize, u32 valueSize, EAlloc allocator);
void dict_del(dict_t* dict);

void dict_clear(dict_t* dict);
void dict_reserve(dict_t* dict, i32 count);

i32 dict_find(const dict_t* dict, const void* key);
bool dict_get(dict_t* dict, const void* key, void* valueOut);
bool dict_set(dict_t* dict, const void* key, const void* valueIn);
bool dict_add(dict_t* dict, const void* key, const void* valueIn);
bool dict_rm(dict_t* dict, const void* key, void* valueOut);

typedef i32(*DictCmpFn)(
    const void* lkey, const void* rkey,
    const void* lval, const void* rval,
    void* usr);

u32* dict_sort(const dict_t* dict, DictCmpFn cmp, void* usr);

PIM_C_END
