#pragma once

#include "common/macro.h"
#include "containers/queue_i32.h"
#include "containers/genid.h"
#include "common/guid.h"

PIM_C_BEGIN

typedef struct Table_s
{
    i32 width;
    i32 valueSize;
    u8* pim_noalias versions;
    void* pim_noalias values;
    i32* pim_noalias refcounts;
    Guid* pim_noalias names;
    queue_i32_t freelist;

    u32 lookupWidth;
    i32 itemCount;
    i32* pim_noalias lookup;
} Table;

void table_new(Table *const table, i32 valueSize);
void table_del(Table *const table);

void table_clear(Table *const table);

bool table_exists(Table const *const table, GenId id);

bool table_add(Table *const table, Guid name, const void *const valueIn, GenId *const idOut);
bool table_retain(Table *const table, GenId id);
bool table_release(Table *const table, GenId id, void *const valueOut);

void *const table_get(const Table *const table, GenId id);

bool table_find(const Table *const table, Guid name, GenId *const idOut);
bool table_getname(const Table *const table, GenId id, Guid *const nameOut);

PIM_C_END
