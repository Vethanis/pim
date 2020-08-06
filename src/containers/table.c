#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/find.h"
#include "common/stringutil.h"
#include "common/nextpow2.h"
#include "math/int2_funcs.h"
#include <string.h>

static void LookupReserve(table_t* table, i32 capacity)
{
    const u32 newWidth = NextPow2(capacity);
    const u32 oldWidth = table->lookupWidth;
    if (newWidth <= oldWidth)
    {
        return;
    }

    i32* newIndices = perm_malloc(sizeof(newIndices[0]) * newWidth);
    i32* oldIndices = table->lookup;

    for (u32 i = 0; i < newWidth; ++i)
    {
        newIndices[i] = -1;
    }

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth; ++i)
    {
        i32 index = oldIndices[i];
        if (index == -1)
        {
            continue;
        }

        u32 hash = table->hashes[index];
        u32 j = hash;
        bool inserted = false;
        while (!inserted)
        {
            j &= newMask;
            if (newIndices[j] < 0)
            {
                newIndices[j] = index;
                inserted = true;
            }
            ++j;
        }
    }

    table->lookup = newIndices;
    table->lookupWidth = newWidth;

    pim_free(oldIndices);
}

static i32 LookupInsert(table_t* table, i32 iTable)
{
    ASSERT(iTable >= 0);
    ASSERT(iTable < table->width);
    LookupReserve(table, table->itemCount + 1);
    table->itemCount += 1;

    const u32 mask = table->lookupWidth - 1u;
    const u32 hash = table->hashes[iTable];
    i32* pim_noalias lookup = table->lookup;
    u32 iLookup = hash;
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

static bool LookupFind(
    const table_t* table,
    const char* name,
    u32 hash,
    i32* pim_noalias iTableOut,
    i32* pim_noalias iLookupOut)
{
    ASSERT(name);
    ASSERT(name[0]);

    *iTableOut = -1;
    *iLookupOut = -1;

    const u32 width = table->lookupWidth;
    const u32 mask = width - 1u;

    const u32* pim_noalias hashes = table->hashes;
    const char** pim_noalias names = table->names;
    const i32* pim_noalias lookup = table->lookup;

    for (u32 i = 0; i < width; ++i)
    {
        const u32 iLookup = (hash + i) & mask;
        const i32 iTable = lookup[iLookup];
        // if empty
        if (iTable == -1)
        {
            break;
        }
        // if not a tombstone
        if (iTable >= 0)
        {
            ASSERT(iTable < table->width);
            if (hash == hashes[iTable])
            {
                if (StrCmp(name, PIM_PATH, names[iTable]) == 0)
                {
                    *iTableOut = iTable;
                    *iLookupOut = (i32)iLookup;
                    return true;
                }
            }
        }
    }

    return false;
}

static void LookupRemove(table_t* table, i32 iTable, i32 iLookup)
{
    if (iLookup >= 0)
    {
        ASSERT(iTable >= 0);
        ASSERT((u32)iLookup < table->lookupWidth);
        // tombstone is anything less than -1
        table->lookup[iLookup] = -2;
        table->itemCount -= 1;
    }
}

static void LookupClear(table_t* table)
{
    u32 width = table->lookupWidth;
    i32* pim_noalias lookup = table->lookup;
    for (u32 i = 0; i < width; ++i)
    {
        lookup[i] = -1;
    }
}

void table_new(table_t* table, i32 valueSize)
{
    ASSERT(table);
    ASSERT(valueSize > 0);

    memset(table, 0, sizeof(*table));
    table->valueSize = valueSize;
    queue_create(&table->freelist, sizeof(i32), EAlloc_Perm);
}

void table_del(table_t* table)
{
    if (table)
    {
        queue_destroy(&table->freelist);
        pim_free(table->versions);
        pim_free(table->values);
        pim_free(table->refcounts);
        pim_free(table->hashes);
        for (i32 i = 0; i < table->width; ++i)
        {
            pim_free(table->names[i]);
            table->names[i] = NULL;
        }
        pim_free(table->names);
        pim_free(table->lookup);
        memset(table, 0, sizeof(*table));
    }
}

void table_clear(table_t* table)
{
    i32 len = table->width;
    table->width = 0;
    table->itemCount = 0;
    memset(table->versions, 0, sizeof(table->versions[0]) * len);
    memset(table->refcounts, 0, sizeof(table->refcounts[0]) * len);
    memset(table->hashes, 0, sizeof(table->hashes[0]) * len);
    for (i32 i = 0; i < len; ++i)
    {
        pim_free(table->names[i]);
        table->names[i] = NULL;
    }
    queue_clear(&table->freelist);
    LookupClear(table);
}

bool table_exists(const table_t* table, genid id)
{
    ASSERT(table->valueSize > 0);
    return (id.index < table->width) && (table->versions[id.index] == id.version);
}

bool table_add(table_t* table, const char* name, const void* valueIn, genid* idOut)
{
    ASSERT(table->valueSize > 0);
    ASSERT(valueIn);
    ASSERT(idOut);

    genid id = { 0 };

    if (!name || !name[0])
    {
        *idOut = id;
        return false;
    }

    if (table_find(table, name, idOut))
    {
        table_retain(table, *idOut);
        return false;
    }

    const i32 stride = table->valueSize;
    if (!queue_trypop(&table->freelist, &id.index, sizeof(id.index)))
    {
        table->width += 1;
        i32 len = table->width;
        PermReserve(table->versions, len);
        PermReserve(table->refcounts, len);
        PermReserve(table->hashes, len);
        PermReserve(table->names, len);
        table->values = perm_realloc(table->values, len * stride);

        id.index = len - 1;
        id.version = 1;
        table->versions[id.index] = id.version;
    }
    ASSERT(id.index >= 0);
    ASSERT(id.index < table->width);

    id.version = table->versions[id.index];
    table->refcounts[id.index] = 1;
    table->hashes[id.index] = HashStr(name);
    table->names[id.index] = StrDup(name, EAlloc_Perm);

    u8* pValues = table->values;
    memcpy(pValues + id.index * stride, valueIn, stride);

    *idOut = id;

    LookupInsert(table, id.index);

    return true;
}

bool table_retain(table_t* table, genid id)
{
    if (table_exists(table, id))
    {
        ASSERT(table->refcounts[id.index] > 0);
        table->refcounts[id.index] += 1;
        return true;
    }
    return false;
}

bool table_release(table_t* table, genid id, void* valueOut)
{
    if (table_exists(table, id))
    {
        ASSERT(table->refcounts[id.index] > 0);
        table->refcounts[id.index] -= 1;
        if (table->refcounts[id.index] == 0)
        {
            ASSERT(table->names[id.index]);
            ASSERT(table->hashes[id.index]);

            i32 iTable, iLookup;
            bool found = LookupFind(
                table,
                table->names[id.index],
                table->hashes[id.index],
                &iTable,
                &iLookup);
            ASSERT(found);
            ASSERT(iTable == id.index);

            LookupRemove(table, iTable, iLookup);

            pim_free(table->names[id.index]);
            table->names[id.index] = NULL;
            table->hashes[id.index] = 0u;
            table->versions[id.index] += 1;
            {
                i32 stride = table->valueSize;
                u8* pValue = table->values;
                pValue += id.index * stride;
                if (valueOut)
                {
                    memcpy(valueOut, pValue, stride);
                }
                memset(pValue, 0, stride);
            }

            queue_push(&table->freelist, &id.index, sizeof(id.index));

            return true;
        }
    }
    return false;
}

bool table_get(const table_t* table, genid id, void* valueOut)
{
    ASSERT(valueOut);
    if (table_exists(table, id))
    {
        i32 stride = table->valueSize;
        const u8* pValues = table->values;
        memcpy(valueOut, pValues + stride * id.index, stride);
        return true;
    }
    return false;
}

bool table_set(table_t* table, genid id, const void* valueIn)
{
    ASSERT(valueIn);
    if (table_exists(table, id))
    {
        i32 stride = table->valueSize;
        u8* pValues = table->values;
        memcpy(pValues + stride * id.index, valueIn, stride);
        return true;
    }
    return false;
}

bool table_find(const table_t* table, const char* name, genid* idOut)
{
    ASSERT(table);
    ASSERT(idOut);

    idOut->index = 0;
    idOut->version = 0;

    if (!name || !name[0])
    {
        return false;
    }

    const u32 hash = HashStr(name);
    i32 iTable, iLookup;
    if (LookupFind(table, name, hash, &iTable, &iLookup))
    {
        idOut->index = iTable;
        idOut->version = table->versions[iTable];
        return true;
    }
    return false;
}
