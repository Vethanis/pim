#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct idtable_s
{
    u32* keys;
    i32* values;
    u32 width;
    u32 count;
    EAlloc allocator;
} idtable_t;

void idtable_new(idtable_t* table, EAlloc allocator);
void idtable_del(idtable_t* table);

void idtable_clear(idtable_t* table);
void idtable_reserve(idtable_t* table, i32 count);

i32 idtable_find(const idtable_t* table, u32 key);
bool idtable_get(const idtable_t* table, u32 key, i32* valueOut);
bool idtable_set(idtable_t* table, u32 key, i32 value);
bool idtable_add(idtable_t* table, u32 key, i32 value);
bool idtable_rm(idtable_t* table, u32 key, i32* valueOut);

PIM_C_END
