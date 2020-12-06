#pragma once

#include "common/macro.h"
#include "containers/queue.h"
#include "common/guid.h"

PIM_C_BEGIN

typedef struct genid_s
{
    u32 version : 8;
    u32 index : 24;
} genid;

typedef struct table_s
{
    i32 width;
    i32 valueSize;
    u8* versions;
    void* values;
    i32* refcounts;
    guid_t* names;
    queue_t freelist;

    u32 lookupWidth;
    i32 itemCount;
    i32* lookup;
} table_t;

void table_new(table_t *const table, i32 valueSize);
void table_del(table_t *const table);

void table_clear(table_t *const table);

bool table_exists(const table_t *const table, genid id);

bool table_add(table_t *const table, guid_t name, const void *const valueIn, genid *const idOut);
bool table_retain(table_t *const table, genid id);
bool table_release(table_t *const table, genid id, void *const valueOut);

void *const table_get(const table_t *const table, genid id);

bool table_find(const table_t *const table, guid_t name, genid *const idOut);
bool table_getname(const table_t *const table, genid id, guid_t *const nameOut);

PIM_C_END
