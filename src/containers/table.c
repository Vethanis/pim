#include "containers/table.h"
#include "allocator/allocator.h"
#include <string.h>

void table_new(table_t* table, i32 valueSize)
{
    ASSERT(valueSize > 0);

    dict_new(&table->lookup, sizeof(guid_t), sizeof(i32), EAlloc_Perm);
    intQ_create(&table->freelist, EAlloc_Perm);
    table->width = 0;
    table->valueSize = valueSize;
    table->versions = NULL;
    table->refcounts = NULL;
    table->names = NULL;
    table->values = NULL;
}

void table_del(table_t* table)
{
    if (table)
    {
        dict_del(&table->lookup);
        intQ_destroy(&table->freelist);
        pim_free(table->versions);
        pim_free(table->refcounts);
        pim_free(table->names);
        pim_free(table->values);
        memset(table, 0, sizeof(*table));
    }
}

void table_clear(table_t* table)
{
    i32 len = table->width;
    table->width = 0;
    memset(table->versions, 0, sizeof(table->versions[0]) * len);
    memset(table->refcounts, 0, sizeof(table->refcounts[0]) * len);
    memset(table->names, 0, sizeof(table->names[0]) * len);
    dict_clear(&table->lookup);
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

    genid id = { 0 };

    if (!name || !name[0])
    {
        *idOut = id;
        return false;
    }

    guid_t key = guid_str(name, guid_seed);
    if (dict_get(&table->lookup, &key, &id))
    {
        *idOut = id;
        return false;
    }

    const i32 stride = table->valueSize;
    if (!intQ_trypop(&table->freelist, &id.index))
    {
        table->width += 1;
        i32 len = table->width;
        PermReserve(table->versions, len);
        PermReserve(table->refcounts, len);
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
    table->names[id.index] = key;

    u8* pValues = table->values;
    memcpy(pValues + id.index * stride, valueIn, stride);

    bool added = dict_add(&table->lookup, &key, &id);
    ASSERT(added);

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
            bool removed = dict_rm(&table->lookup, table->names + id.index, NULL);
            ASSERT(removed);

            table->versions[id.index] += 1;
            table->names[id.index] = (guid_t) { 0 };
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

genid table_lookup(const table_t* table, const char* name)
{
    guid_t key = guid_str(name, guid_seed);
    genid id = { 0 };
    if (dict_get(&table->lookup, &key, &id.index))
    {
        ASSERT(id.index >= 0);
        ASSERT(id.index < table->width);
        id.version = table->versions[id.index];
    }
    return id;
}
