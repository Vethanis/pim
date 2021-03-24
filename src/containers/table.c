#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/find.h"
#include "common/stringutil.h"
#include "common/nextpow2.h"
#include "math/int2_funcs.h"
#include <string.h>

void Table_New(Table *const table, i32 valueSize)
{
    ASSERT(table);
    ASSERT(valueSize > 0);

    memset(table, 0, sizeof(*table));
    table->valueSize = valueSize;
    Lookup_New(&table->lookup);
}

void Table_Del(Table *const table)
{
    if (table)
    {
        IntQueue_Del(&table->freelist);
        Lookup_Del(&table->lookup);
        Mem_Free(table->versions);
        Mem_Free(table->values);
        Mem_Free(table->refcounts);
        Mem_Free(table->names);
        memset(table, 0, sizeof(*table));
    }
}

void Table_Clear(Table *const table)
{
    const i32 len = table->width;
    table->width = 0;
    table->itemCount = 0;
    memset(table->versions, 0, sizeof(table->versions[0]) * len);
    memset(table->refcounts, 0, sizeof(table->refcounts[0]) * len);
    memset(table->names, 0, sizeof(table->names[0]) * len);
    IntQueue_Clear(&table->freelist);
    Lookup_Clear(&table->lookup);
}

bool Table_Exists(Table const *const table, GenId id)
{
    ASSERT(table->valueSize > 0);
    i32 i = id.index;
    u8 v = id.version;
    return (i < table->width) && (table->versions[i] == v);
}

bool Table_Add(
    Table *const table,
    Guid name,
    const void *const valueIn,
    GenId *const idOut)
{
    ASSERT(table->valueSize > 0);
    ASSERT(valueIn);
    ASSERT(idOut);

    if (Guid_IsNull(name))
    {
        idOut->index = 0;
        idOut->version = 0;
        return false;
    }

    if (Table_Find(table, name, idOut))
    {
        Table_Retain(table, *idOut);
        return false;
    }

    const i32 stride = table->valueSize;
    i32 index = 0;
    u8 version = 0;
    if (!IntQueue_TryPop(&table->freelist, &index))
    {
        table->width += 1;
        const i32 len = table->width;
        Perm_Reserve(table->versions, len);
        Perm_Reserve(table->refcounts, len);
        Perm_Reserve(table->names, len);
        table->values = Perm_Realloc(table->values, len * stride);

        index = len - 1;
        version = 1;
        table->versions[index] = version;
    }
    ASSERT(index >= 0);
    ASSERT(index < table->width);

    version = table->versions[index];
    table->refcounts[index] = 1;
    table->names[index] = name;

    u8 *const pim_noalias pValues = table->values;
    memcpy(pValues + index * stride, valueIn, stride);

    Lookup_Insert(&table->lookup, table->names, table->width, name, index);

    table->itemCount += 1;
    idOut->index = index;
    idOut->version = version;

    return true;
}

bool Table_Retain(Table *const table, GenId id)
{
    if (Table_Exists(table, id))
    {
        ASSERT(table->refcounts[id.index] > 0);
        table->refcounts[id.index] += 1;
        return true;
    }
    return false;
}

bool Table_Release(Table *const table, GenId id, void *const valueOut)
{
    if (Table_Exists(table, id))
    {
        const i32 index = id.index;
        const i32 version = id.version;

        ASSERT(table->refcounts[index] > 0);
        table->refcounts[index] -= 1;
        if (table->refcounts[index] == 0)
        {
            ASSERT(!Guid_IsNull(table->names[index]));

            i32 iName, iLookup;
            bool found = Lookup_Find(
                &table->lookup,
                table->names,
                table->width,
                table->names[index],
                &iName,
                &iLookup);
            ASSERT(found);
            ASSERT(iName == index);

            Lookup_Remove(&table->lookup, iName, iLookup);

            table->names[index] = (Guid) { 0 };
            table->versions[index] += 1;
            {
                i32 stride = table->valueSize;
                u8* pim_noalias pValue = table->values;
                pValue += index * stride;
                if (valueOut)
                {
                    memcpy(valueOut, pValue, stride);
                }
                memset(pValue, 0, stride);
            }

            IntQueue_Push(&table->freelist, index);

            table->itemCount -= 1;
            return true;
        }
    }
    return false;
}

void *const Table_Get(const Table *const table, GenId id)
{
    if (Table_Exists(table, id))
    {
        i32 index = id.index;
        i32 stride = table->valueSize;
        u8 *const values = table->values;
        void *const ptr = values + stride * index;
        return ptr;
    }
    return NULL;
}

bool Table_Find(const Table *const table, Guid name, GenId *const idOut)
{
    ASSERT(table);
    ASSERT(idOut);
    idOut->index = 0;
    idOut->version = 0;
    i32 iName, iLookup;
    if (Lookup_Find(
        &table->lookup,
        table->names,
        table->width,
        name,
        &iName,
        &iLookup))
    {
        idOut->index = iName;
        idOut->version = table->versions[iName];
        return true;
    }
    return false;
}

bool Table_GetName(const Table *const table, GenId id, Guid *const nameOut)
{
    ASSERT(nameOut);
    nameOut->a = 0;
    nameOut->b = 0;
    if (Table_Exists(table, id))
    {
        i32 index = id.index;
        *nameOut = table->names[index];
        return true;
    }
    return false;
}
