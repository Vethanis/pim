#pragma once

#include "common/macro.h"
#include "containers/queue_i32.h"
#include "containers/genid.h"
#include "common/guid.h"

PIM_C_BEGIN

typedef struct table_s
{
    i32 width;
    i32 valueSize;
    u8* pim_noalias versions;
    void* pim_noalias values;
    i32* pim_noalias refcounts;
    guid_t* pim_noalias names;
    queue_i32_t freelist;

    u32 lookupWidth;
    i32 itemCount;
    i32* pim_noalias lookup;
} table_t;

void table_new(table_t *const table, i32 valueSize);
void table_del(table_t *const table);

void table_clear(table_t *const table);

bool table_exists(table_t const *const table, genid_t id);

bool table_add(table_t *const table, guid_t name, const void *const valueIn, genid_t *const idOut);
bool table_retain(table_t *const table, genid_t id);
bool table_release(table_t *const table, genid_t id, void *const valueOut);

void *const table_get(const table_t *const table, genid_t id);

bool table_find(const table_t *const table, guid_t name, genid_t *const idOut);
bool table_getname(const table_t *const table, genid_t id, guid_t *const nameOut);

PIM_C_END
