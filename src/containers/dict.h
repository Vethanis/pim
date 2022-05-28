#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Dict_s
{
    u32 * pim_noalias hashes;
    void * pim_noalias keys;
    void * pim_noalias values;
    u32 width;
    i32 count;
    u32 keySize;
    u32 valueSize;
    EAlloc allocator;
} Dict;

// initialize dictionary before using
void Dict_New(Dict* dict, u32 keySize, u32 valueSize, EAlloc allocator);
// release storage
void Dict_Del(Dict* dict);

// removes all entries, but doesn't affect storage
void Dict_Clear(Dict* dict);
// resizes table if necessary. affects order and width.
void Dict_Reserve(Dict* dict, i32 count);

// number of items in dictionary
i32 Dict_GetCount(const Dict* dict);
// find slot of a key, or -1 if it does not exist
i32 Dict_Find(const Dict* dict, const void* key);
// copies to valueOut if key exists
bool Dict_Get(const Dict* dict, const void* key, void* valueOut);
// overwrites value if key exists
bool Dict_Set(Dict* dict, const void* key, const void* valueIn);
// adds a key and value if it does not exist
bool Dict_Add(Dict* dict, const void* key, const void* valueIn);
// removes a key and value if it exists, optional copy to valueOut
bool Dict_Rm(Dict* dict, const void* key, void* valueOut);
// overwrites a value if key exists, otherwise adds key and value
bool Dict_SetAdd(Dict* dict, const void* key, const void* valueIn);
// copies to valueOut if key exists, otherwise adds key and value
bool Dict_GetAdd(Dict* dict, const void* key, void* valueInOut);

typedef i32(*DictCmpFn)(
    const void* lkey, const void* rkey,
    const void* lval, const void* rval,
    void* usr);
// returns a temporary array of sorted indices into the dict.
// array length matches Dict_GetCount().
u32* Dict_Sort(const Dict* dict, DictCmpFn cmp, void* usr);

// number of slots, for table entry iteration.
u32 Dict_GetWidth(const Dict* dict);
// does table slot have a valid entry
bool Dict_ValidAt(const Dict* dict, u32 slot);
bool Dict_GetKeyAt(const Dict* dict, u32 slot, void* keyOut);
bool Dict_GetValueAt(const Dict* dict, u32 slot, void* valueOut);
bool Dict_SetValueAt(Dict* dict, u32 slot, const void* valueIn);
bool Dict_GetAt(const Dict* dict, u32 slot, void* keyOut, void* valueOut);
// removes a key and value at slot if it exists.
bool Dict_RmAt(Dict* dict, u32 slot, void* valueOut);

PIM_C_END
