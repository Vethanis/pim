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
    IntQueue freelist;

    u32 lookupWidth;
    i32 itemCount;
    i32* pim_noalias lookup;
} Table;

void Table_New(Table *const table, i32 valueSize);
void Table_Del(Table *const table);

void Table_Clear(Table *const table);

bool Table_Exists(Table const *const table, GenId id);

bool Table_Add(Table *const table, Guid name, const void *const valueIn, GenId *const idOut);
bool Table_Retain(Table *const table, GenId id);
bool Table_Release(Table *const table, GenId id, void *const valueOut);

void *const Table_Get(const Table *const table, GenId id);

bool Table_Find(const Table *const table, Guid name, GenId *const idOut);
bool Table_GetName(const Table *const table, GenId id, Guid *const nameOut);

PIM_C_END
