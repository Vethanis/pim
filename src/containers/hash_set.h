#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct hashset_s
{
    u32* hashes;
    void* keys;
    u32 count;
    u32 width;
    u32 stride;
    EAlloc allocator;
} hashset_t;

void hashset_new(hashset_t* set, u32 keySize, EAlloc allocator);
void hashset_del(hashset_t* set);
void hashset_clear(hashset_t* set);
void hashset_reserve(hashset_t* set, u32 minCount);
bool hashset_contains(const hashset_t* set, const void* key, u32 keySize);
bool hashset_add(hashset_t* set, const void* key, u32 keySize);
bool hashset_rm(hashset_t* set, const void* key, u32 keySize);

PIM_C_END
