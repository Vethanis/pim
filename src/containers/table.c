#include "containers/table.h"
#include "allocator/allocator.h"
#include "common/fnv1a.h"
#include "common/find.h"
#include "common/stringutil.h"
#include <string.h>

void table_new(table_t* table, i32 valueSize)
{
    ASSERT(table);
    ASSERT(valueSize > 0);

    memset(table, 0, sizeof(*table));
    table->valueSize = valueSize;
    intQ_create(&table->freelist, EAlloc_Perm);
}

void table_del(table_t* table)
{
    if (table)
    {
        intQ_destroy(&table->freelist);
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
        memset(table, 0, sizeof(*table));
    }
}

void table_clear(table_t* table)
{
    i32 len = table->width;
    table->width = 0;
    memset(table->versions, 0, sizeof(table->versions[0]) * len);
    memset(table->refcounts, 0, sizeof(table->refcounts[0]) * len);
    memset(table->hashes, 0, sizeof(table->hashes[0]) * len);
    for (i32 i = 0; i < len; ++i)
    {
        pim_free(table->names[i]);
        table->names[i] = NULL;
    }
    intQ_clear(&table->freelist);
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

    genid id = { -1, 0 };

    if (!name || !name[0])
    {
        *idOut = id;
        return false;
    }

    if (table_find(table, name, idOut))
    {
        return false;
    }

    const i32 stride = table->valueSize;
    if (!intQ_trypop(&table->freelist, &id.index))
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

            intQ_push(&table->freelist, id.index);

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

    idOut->index = -1;
    idOut->version = 0;

    if (!name || !name[0])
    {
        return false;
    }
    const u32 hash = HashStr(name);
    const u32* hashes = table->hashes;
    const char** names = table->names;
    const i32 width = table->width;
    for (i32 i = width - 1; i >= 0; --i)
    {
        if (hashes[i] == hash)
        {
            if (StrICmp(name, PIM_PATH, names[i]) == 0)
            {
                idOut->index = i;
                idOut->version = table->versions[i];
                return true;
            }
        }
    }
    return false;
}
