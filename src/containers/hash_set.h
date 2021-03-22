#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct HashSet_s
{
    u32* hashes;
    void* keys;
    u32 count;
    u32 width;
    u32 stride;
    EAlloc allocator;
} HashSet;

void HashSet_New(HashSet* set, u32 keySize, EAlloc allocator);
void HashSet_Del(HashSet* set);
void HashSet_Clear(HashSet* set);
void HashSet_Reserve(HashSet* set, u32 minCount);
bool HashSet_Contains(const HashSet* set, const void* key, u32 keySize);
bool HashSet_Add(HashSet* set, const void* key, u32 keySize);
bool HashSet_Rm(HashSet* set, const void* key, u32 keySize);

PIM_C_END
