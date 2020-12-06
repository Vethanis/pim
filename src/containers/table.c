#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/find.h"
#include "common/stringutil.h"
#include "common/nextpow2.h"
#include "math/int2_funcs.h"
#include <string.h>

#define kEmpty  -1
#define kTomb   -2

pim_inline void LookupReserve(table_t *const table, i32 capacity)
{
    const u32 newWidth = NextPow2(capacity) * 4;
    const u32 oldWidth = table->lookupWidth;
    if (newWidth <= oldWidth)
    {
        return;
    }

    i32 *const pim_noalias newIndices = perm_malloc(sizeof(newIndices[0]) * newWidth);
    i32 *const pim_noalias oldIndices = table->lookup;
    const guid_t *const pim_noalias names = table->names;

    for (u32 i = 0; i < newWidth; ++i)
    {
        newIndices[i] = kEmpty;
    }

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth; ++i)
    {
        i32 index = oldIndices[i];
        if (index >= 0)
        {
            u32 j = guid_hashof(names[index]);
            while (true)
            {
                j &= newMask;
                if (newIndices[j] < 0)
                {
                    newIndices[j] = index;
                    break;
                }
                ++j;
            }
        }
    }

    table->lookup = newIndices;
    table->lookupWidth = newWidth;

    pim_free(oldIndices);
}

pim_inline i32 LookupInsert(table_t *const table, i32 iTable, guid_t name)
{
    ASSERT(iTable >= 0);
    ASSERT(iTable < table->width);
    LookupReserve(table, table->itemCount + 1);
    table->itemCount += 1;

    const u32 mask = table->lookupWidth - 1u;
    i32 *const pim_noalias lookup = table->lookup;
    u32 iLookup = guid_hashof(name);
    while (true)
    {
        iLookup &= mask;
        // if empty or tombstone
        if (lookup[iLookup] < 0)
        {
            lookup[iLookup] = iTable;
            return (i32)iLookup;
        }
        ++iLookup;
    }
    return -1;
}

pim_inline bool LookupFind(
    const table_t *const table,
    guid_t name,
    i32 *const pim_noalias iTableOut,
    i32 *const pim_noalias iLookupOut)
{
    ASSERT(!guid_isnull(name));

    *iTableOut = -1;
    *iLookupOut = -1;

    const u32 width = table->lookupWidth;
    const u32 mask = width - 1u;

    const guid_t *const pim_noalias names = table->names;
    const i32 *const pim_noalias lookup = table->lookup;
    const u32 hash = guid_hashof(name);

    for (u32 i = 0; i < width; ++i)
    {
        const u32 iLookup = (hash + i) & mask;
        const i32 iTable = lookup[iLookup];
        // if empty
        if (iTable == kEmpty)
        {
            break;
        }
        // if not a tombstone
        if (iTable >= 0)
        {
            ASSERT(iTable < table->width);
            if (guid_eq(name, names[iTable]))
            {
                *iTableOut = iTable;
                *iLookupOut = (i32)iLookup;
                return true;
            }
        }
    }

    return false;
}

pim_inline void LookupRemove(table_t *const table, i32 iTable, i32 iLookup)
{
    if (iLookup >= 0)
    {
        ASSERT(iTable >= 0);
        ASSERT((u32)iLookup < table->lookupWidth);
        table->lookup[iLookup] = kTomb;
        table->itemCount -= 1;
    }
}

pim_inline void LookupClear(table_t *const table)
{
    const u32 width = table->lookupWidth;
    i32 *const pim_noalias lookup = table->lookup;
    for (u32 i = 0; i < width; ++i)
    {
        lookup[i] = kEmpty;
    }
}

void table_new(table_t *const table, i32 valueSize)
{
    ASSERT(table);
    ASSERT(valueSize > 0);

    memset(table, 0, sizeof(*table));
    table->valueSize = valueSize;
    queue_create(&table->freelist, sizeof(i32), EAlloc_Perm);
}

void table_del(table_t *const table)
{
    if (table)
    {
        queue_destroy(&table->freelist);
        pim_free(table->versions);
        pim_free(table->values);
        pim_free(table->refcounts);
        pim_free(table->names);
        pim_free(table->lookup);
        memset(table, 0, sizeof(*table));
    }
}

void table_clear(table_t *const table)
{
    const i32 len = table->width;
    table->width = 0;
    table->itemCount = 0;
    memset(table->versions, 0, sizeof(table->versions[0]) * len);
    memset(table->refcounts, 0, sizeof(table->refcounts[0]) * len);
    memset(table->names, 0, sizeof(table->names[0]) * len);
    queue_clear(&table->freelist);
    LookupClear(table);
}

bool table_exists(const table_t *const table, genid id)
{
    const u8 *const pim_noalias versions = table->versions;
    i32 index = id.index;
    u8 version = id.version;
    ASSERT(table->valueSize > 0);
    ASSERT(index < table->width);
    return versions[index] == version;
}

bool table_add(
    table_t *const table,
    guid_t name,
    const void *const valueIn,
    genid *const idOut)
{
    ASSERT(table->valueSize > 0);
    ASSERT(valueIn);
    ASSERT(idOut);

    if (guid_isnull(name))
    {
        idOut->index = 0;
        idOut->version = 0;
        return false;
    }

    if (table_find(table, name, idOut))
    {
        table_retain(table, *idOut);
        return false;
    }

    const i32 stride = table->valueSize;
    i32 index = 0;
    u8 version = 0;
    if (!queue_trypop(&table->freelist, &index, sizeof(index)))
    {
        table->width += 1;
        const i32 len = table->width;
        PermReserve(table->versions, len);
        PermReserve(table->refcounts, len);
        PermReserve(table->names, len);
        table->values = perm_realloc(table->values, len * stride);

        index = len - 1;
        version = 1;
        table->versions[index] = version;
    }
    ASSERT(index >= 0);
    ASSERT(index < table->width);

    version = table->versions[index];
    table->refcounts[index] = 1;
    table->names[index] = name;

    u8 *const pValues = table->values;
    memcpy(pValues + index * stride, valueIn, stride);

    LookupInsert(table, index, name);

    idOut->index = index;
    idOut->version = version;

    return true;
}

bool table_retain(table_t *const table, genid id)
{
    if (table_exists(table, id))
    {
        ASSERT(table->refcounts[id.index] > 0);
        table->refcounts[id.index] += 1;
        return true;
    }
    return false;
}

bool table_release(table_t *const table, genid id, void *const valueOut)
{
    if (table_exists(table, id))
    {
        const i32 index = id.index;
        const i32 version = id.version;

        ASSERT(table->refcounts[index] > 0);
        table->refcounts[index] -= 1;
        if (table->refcounts[index] == 0)
        {
            ASSERT(!guid_isnull(table->names[index]));

            i32 iTable, iLookup;
            bool found = LookupFind(
                table,
                table->names[index],
                &iTable,
                &iLookup);
            ASSERT(found);
            ASSERT(iTable == index);

            LookupRemove(table, iTable, iLookup);

            table->names[index] = (guid_t) { 0 };
            table->versions[index] += 1;
            {
                i32 stride = table->valueSize;
                u8* pValue = table->values;
                pValue += index * stride;
                if (valueOut)
                {
                    memcpy(valueOut, pValue, stride);
                }
                memset(pValue, 0, stride);
            }

            queue_push(&table->freelist, &index, sizeof(index));

            return true;
        }
    }
    return false;
}

void *const table_get(const table_t *const table, genid id)
{
    if (table_exists(table, id))
    {
        i32 index = id.index;
        i32 stride = table->valueSize;
        u8 *const values = table->values;
        void *const ptr = values + stride * index;
        return ptr;
    }
    return NULL;
}

bool table_find(const table_t *const table, guid_t name, genid *const idOut)
{
    ASSERT(table);
    ASSERT(idOut);

    idOut->index = 0;
    idOut->version = 0;

    if (guid_isnull(name))
    {
        return false;
    }

    i32 iTable, iLookup;
    if (LookupFind(table, name, &iTable, &iLookup))
    {
        idOut->index = iTable;
        idOut->version = table->versions[iTable];
        return true;
    }
    return false;
}

bool table_getname(const table_t *const table, genid id, guid_t *const nameOut)
{
    ASSERT(nameOut);
    nameOut->a = 0;
    nameOut->b = 0;
    if (table_exists(table, id))
    {
        i32 index = id.index;
        *nameOut = table->names[index];
        return true;
    }
    return false;
}
