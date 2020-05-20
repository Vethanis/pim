#include "containers/idtable.h"
#include "allocator/allocator.h"
#include "containers/hash_util.h"
#include <string.h>

static bool empty_slot(u32 key)
{
    return (key == 0u) | (key & hashutil_tomb_mask);
}

void idtable_new(idtable_t* table, EAlloc allocator)
{
    ASSERT(table);
    memset(table, 0, sizeof(*table));
    table->allocator = allocator;
}

void idtable_del(idtable_t* table)
{
    ASSERT(table);
    pim_free(table->keys);
    pim_free(table->values);
    memset(table, 0, sizeof(*table));
}

void idtable_clear(idtable_t* table)
{
    ASSERT(table);
    table->count = 0;
    memset(table->keys, 0, sizeof(table->keys[0]) * table->width);
}

void idtable_reserve(idtable_t* table, i32 count)
{
    ASSERT(table);

    count = count > 16 ? count : 16;
    const u32 newWidth = NextPow2((u32)count);
    const u32 oldWidth = table->width;
    if (newWidth <= oldWidth)
    {
        return;
    }

    EAlloc allocator = table->allocator;
    u32* pim_noalias newKeys = pim_calloc(allocator, sizeof(newKeys[0]) * newWidth);
    i32* pim_noalias newValues = pim_calloc(allocator, sizeof(newValues[0]) * newWidth);

    u32* pim_noalias oldKeys = table->keys;
    i32* pim_noalias oldValues = table->values;

    const u32 newMask = newWidth - 1u;
    for (u32 i = 0; i < oldWidth;)
    {
        u32 key = oldKeys[i];
        i32 value = oldValues[i];
        if (key)
        {
            u32 j = key;
            while (true)
            {
                j &= newMask;
                if (!newKeys[j])
                {
                    newKeys[j] = key;
                    newValues[j] = value;
                    goto next;
                }
                ++j;
            }
        }
    next:
        ++i;
    }

    pim_free(oldKeys);
    pim_free(oldValues);

    table->width = newWidth;
    table->keys = newKeys;
    table->values = newValues;
}

i32 idtable_find(const idtable_t* table, u32 key)
{
    ASSERT(table);
    if (empty_slot(key))
    {
        return -1;
    }

    u32 width = table->width;
    const u32 mask = width - 1u;
    const u32* pim_noalias keys = table->keys;

    u32 j = key;
    while (width--)
    {
        j &= mask;
        u32 jKey = keys[j];
        if (!jKey)
        {
            break;
        }
        if (key == jKey)
        {
            return (i32)j;
        }
        ++j;
    }

    return -1;
}

bool idtable_get(const idtable_t* table, u32 key, i32* valueOut)
{
    ASSERT(valueOut);

    i32 i = idtable_find(table, key);
    if (i != -1)
    {
        *valueOut = table->values[i];
        return true;
    }
    return false;
}

bool idtable_set(idtable_t* table, u32 key, i32 value)
{
    i32 i = idtable_find(table, key);
    if (i != -1)
    {
        table->values[i] = value;
        return true;
    }
    return false;
}

bool idtable_add(idtable_t* table, u32 key, i32 value)
{
    if (empty_slot(key))
    {
        return false;
    }
    if (idtable_find(table, key) != -1)
    {
        return false;
    }

    idtable_reserve(table, table->count + 1);
    table->count += 1;

    const u32 mask = table->width - 1u;

    u32* pim_noalias keys = table->keys;
    i32* pim_noalias values = table->values;

    u32 j = key;
    while (true)
    {
        j &= mask;
        if (empty_slot(keys[j]))
        {
            keys[j] = key;
            values[j] = value;
            return true;
        }
        ++j;
    }
}

bool idtable_rm(idtable_t* table, u32 key, i32* valueOut)
{
    i32 i = idtable_find(table, key);
    if (i != -1)
    {
        table->keys[i] |= hashutil_tomb_mask;
        if (valueOut)
        {
            *valueOut = table->values[i];
        }
        table->values[i] = 0;
        table->count -= 1;
        return true;
    }
    return false;
}
