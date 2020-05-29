#include "components/table.h"
#include "allocator/allocator.h"
#include "common/find.h"
#include "common/atomics.h"
#include <string.h>

static u32 ms_instanceid;
static tables_t ms_tablesmain;

tables_t* tables_main(void)
{
    return &ms_tablesmain;
}

// ----------------------------------------------------------------------------
// Tables API

tables_t* tables_new(void)
{
    return perm_calloc(sizeof(tables_t));
}

void tables_del(tables_t* tables)
{
    if (tables)
    {
        for (i32 i = 0; i < tables->count; ++i)
        {
            table_del(tables->tables[i]);
        }
        pim_free(tables->tables);
        pim_free(tables->hashes);
        pim_free(tables);
    }
}

table_t* tables_get(tables_t* tables, u32 hash)
{
    ASSERT(tables);
    ASSERT(hash);
    i32 i = Find_u32(tables->hashes, tables->count, hash);
    if (i != -1)
    {
        return tables->tables[i];
    }
    return NULL;
}

bool tables_has(const tables_t* tables, u32 hash)
{
    ASSERT(tables);
    ASSERT(hash);
    return Find_u32(tables->hashes, tables->count, hash) != -1;
}

table_t* tables_add(tables_t* tables, u32 hash)
{
    table_t* table = tables_get(tables, hash);
    if (!table)
    {
        i32 len = tables->count + 1;
        tables->count = len;
        tables->hashes = perm_realloc(tables->hashes, sizeof(tables->hashes[0]) * len);
        tables->tables = perm_realloc(tables->tables, sizeof(tables->tables[0]) * len);

        table = table_new(hash);
        tables->hashes[len - 1] = hash;
        tables->tables[len - 1] = table;
    }
    return table;
}

bool tables_rm(tables_t* tables, u32 hash)
{
    ASSERT(tables);
    ASSERT(hash);
    i32 i = Find_u32(tables->hashes, tables->count, hash);
    if (i != -1)
    {
        table_t* table = tables->tables[i];
        table_del(table);

        i32 b = tables->count - 1;
        tables->count = b;
        tables->hashes[i] = tables->hashes[b];
        tables->tables[i] = tables->tables[b];
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// Table API

table_t* table_new(u32 nameHash)
{
    ASSERT(nameHash);
    table_t* table = perm_calloc(sizeof(table_t));
    table->hash = nameHash;
    idtable_new(&(table->colidt), EAlloc_Perm);
    return table;
}

void table_del(table_t* table)
{
    if (table)
    {
        idtable_del(&(table->colidt));
        for (i32 i = 0; i < table->rowct; ++i)
        {
            pim_free(table->rows[i].ptr);
        }
        pim_free(table->rows);
        pim_free(table->ids);
        pim_free(table->rownames);
        pim_free(table);
    }
}

row_t* table_get(table_t* table, u32 typeHash)
{
    ASSERT(table);
    ASSERT(typeHash);

    i32 i = Find_u32(table->rownames, table->rowct, typeHash);
    if (i != -1)
    {
        return table->rows + i;
    }
    return NULL;
}

bool table_has(const table_t* table, u32 typeHash)
{
    ASSERT(table);
    ASSERT(typeHash);

    return Find_u32(table->rownames, table->rowct, typeHash) != -1;
}

bool table_add(table_t* table, u32 typeHash, i32 typeSize)
{
    ASSERT(table);
    ASSERT(typeHash);
    ASSERT(typeSize > 0);

    i32 i = Find_u32(table->rownames, table->rowct, typeHash);
    if (i == -1)
    {
        row_t row = { 0 };
        row.length = table->columnct;
        row.stride = typeSize;
        row.ptr = perm_calloc(row.length * row.stride);

        const i32 height = table->rowct + 1;
        table->rowct = height;
        table->rownames = perm_realloc(table->rownames, sizeof(table->rownames[0]) * height);
        table->rows = perm_realloc(table->rows, sizeof(table->rows[0]) * height);

        table->rownames[height - 1] = typeHash;
        table->rows[height - 1] = row;

        return true;
    }
    return false;
}

bool table_rm(table_t* table, u32 typeHash)
{
    ASSERT(table);
    ASSERT(typeHash);

    i32 i = Find_u32(table->rownames, table->rowct, typeHash);
    if (i != -1)
    {
        pim_free(table->rows[i].ptr);

        i32 b = table->rowct - 1;
        table->rowct = b;
        table->rows[i] = table->rows[b];
        table->rownames[i] = table->rownames[b];

        return true;
    }
    return false;
}

void* table_row(table_t* table, u32 typeHash, i32 typeSize)
{
    row_t* row = table_get(table, typeHash);
    if (row)
    {
        ASSERT(row->stride == typeSize);
        return row->ptr;
    }
    return NULL;
}

// ----------------------------------------------------------------------------
// Column API

i32 col_get(const table_t* table, ent_t ent)
{
    ASSERT(table);
    ASSERT(ent.id);
    ASSERT(table->hash == ent.table);

    i32 index;
    if (idtable_get(&(table->colidt), ent.id, &index))
    {
        return index;
    }
    return -1;
}

bool col_has(const table_t* table, ent_t ent)
{
    return col_get(table, ent) != -1;
}

i32 col_add(table_t* table, ent_t* entOut)
{
    ASSERT(table);

    const i32 width = ++table->columnct;
    const i32 height = table->rowct;
    const i32 e = width - 1;

    const u32 id = 1u + inc_u32(&ms_instanceid, MO_Relaxed);
    table->ids = perm_realloc(table->ids, sizeof(table->ids[0]) * width);
    table->ids[e] = id;

    row_t* rows = table->rows;
    for (i32 i = 0; i < height; ++i)
    {
        row_t* row = rows + i;
        row->length = width;
        const i32 stride = row->stride;
        ASSERT(stride > 0);

        ASSERT(e * stride < stride * width);
        ASSERT(e * stride >= 0);

        row->ptr = perm_realloc(row->ptr, stride * width);
        memset(row->ptr + e * stride, 0, stride);
    }

    if (!idtable_add(&(table->colidt), id, e))
    {
        ASSERT(false);
        return -1;
    }

    if (entOut)
    {
        entOut->id = id;
        entOut->table = table->hash;
    }
    return e;
}

bool col_rm(table_t* table, ent_t ent)
{
    ASSERT(table);
    ASSERT(table->hash == ent.table);

    i32 e;
    if (idtable_rm(&(table->colidt), ent.id, &e))
    {
        const i32 b = table->columnct - 1;
        table->columnct = b;

        const u32 idb = table->ids[b];
        ASSERT(ent.id == table->ids[e]);
        table->ids[e] = idb;

        if (!idtable_set(&(table->colidt), idb, e))
        {
            ASSERT(false);
            return false;
        }

        const i32 rowct = table->rowct;
        row_t* rows = table->rows;
        for (i32 i = 0; i < rowct; ++i)
        {
            row_t* row = rows + i;
            row->length = b;
            const i32 stride = row->stride;
            ASSERT(stride > 0);

            u8* ptr = row->ptr;
            memcpy(ptr + i * stride, ptr + b * stride, stride);
        }

        return true;
    }
    return false;
}

void row_set(row_t* row, i32 i, const void* src)
{
    ASSERT(row);
    ASSERT(i >= 0);
    ASSERT(i < row->length);
    ASSERT(src);
    const i32 stride = row->stride;
    ASSERT(stride > 0);
    memcpy(row->ptr + i * stride, src, stride);
}

void row_get(const row_t* row, i32 i, void* dst)
{
    ASSERT(row);
    ASSERT(i >= 0);
    ASSERT(i < row->length);
    ASSERT(dst);
    const i32 stride = row->stride;
    ASSERT(stride > 0);
    memcpy(dst, row->ptr + i * stride, stride);
}
