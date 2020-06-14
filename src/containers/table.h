#pragma once

#include "common/macro.h"
#include "common/guid.h"
#include "containers/dict.h"
#include "containers/int_queue.h"

PIM_C_BEGIN

typedef struct genid_s
{
    i32 index;
    i32 version;
} genid;

typedef struct table_s
{
    i32 width;
    i32 valueSize;
    i32* versions;
    void* values;
    i32* refcounts;
    guid_t* names;
    dict_t lookup;
    intQ_t freelist;
} table_t;

void table_new(table_t* table, i32 valueSize);
void table_del(table_t* table);

void table_clear(table_t* table);

bool table_exists(const table_t* table, genid id);

bool table_add(table_t* table, const char* name, const void* valueIn, genid* idOut);
bool table_retain(table_t* table, genid id);
bool table_release(table_t* table, genid id, void* valueOut);

bool table_get(const table_t* table, genid id, void* valueOut);
bool table_set(table_t* table, genid id, const void* valueIn);

genid table_lookup(const table_t* table, const char* name);

PIM_C_END
