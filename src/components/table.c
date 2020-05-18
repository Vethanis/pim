#include "components/table.h"
#include "allocator/allocator.h"
#include "common/find.h"
#include <string.h>

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

table_t* tables_get_h(tables_t* tables, u32 hash)
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

bool tables_has_h(const tables_t* tables, u32 hash)
{
    ASSERT(tables);
    ASSERT(hash);
    return Find_u32(tables->hashes, tables->count, hash) != -1;
}

table_t* tables_add_h(tables_t* tables, u32 hash)
{
    table_t* table = tables_get_h(tables, hash);
    if (!table)
    {
        i32 len = tables->count + 1;
        tables->count = len;
        tables->hashes = perm_realloc(tables->hashes, sizeof(tables->hashes[0]) * len);
        tables->tables = perm_realloc(tables->tables, sizeof(tables->tables[0]) * len);

        table = table_new();
        tables->hashes[len - 1] = hash;
        tables->tables[len - 1] = table;
    }
    return table;
}

bool tables_rm_h(tables_t* tables, u32 hash)
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

table_t* table_new(void)
{
    return perm_calloc(sizeof(table_t));
}
void table_del(table_t* table)
{
    if (table)
    {
        for (i32 i = 0; i < table->rowct; ++i)
        {
            pim_free(table->rows[i].ptr);
        }
        pim_free(table->rows);
        pim_free(table->entnames);
        pim_free(table->rownames);
        pim_free(table);
    }
}

row_t* table_get_h(table_t* table, u32 typeHash)
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

bool table_has_h(const table_t* table, u32 typeHash)
{
    ASSERT(table);
    ASSERT(typeHash);

    return Find_u32(table->rownames, table->rowct, typeHash) != -1;
}

bool table_add_h(table_t* table, u32 typeHash, i32 typeSize)
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

        i32 len = table->rowct + 1;
        table->rowct = len;
        table->rownames = perm_realloc(table->rownames, sizeof(table->rownames[0]) * len);
        table->rows = perm_realloc(table->rows, sizeof(table->rows[0]) * len);

        table->rownames[len - 1] = typeHash;
        table->rows[len - 1] = row;

        return true;
    }
    return false;
}

bool table_rm_h(table_t* table, u32 typeHash)
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

// ----------------------------------------------------------------------------
// Column API

i32 col_get_h(const table_t* table, u32 entName)
{
    ASSERT(table);
    ASSERT(entName);

    return Find_u32(table->entnames, table->columnct, entName);
}

bool col_has_h(const table_t* table, u32 entName)
{
    return col_get_h(table, entName) != -1;
}

i32 col_add_h(table_t* table, u32 entName)
{
    i32 e = col_get_h(table, entName);
    if (e == -1)
    {
        const i32 len = table->columnct + 1;
        table->columnct = len;
        e = len - 1;

        table->entnames = perm_realloc(table->entnames, sizeof(table->entnames[0]) * len);
        table->entnames[e] = entName;

        const i32 rowct = table->rowct;
        row_t* rows = table->rows;
        for (i32 i = 0; i < rowct; ++i)
        {
            rows[i].length = len;
            i32 stride = rows[i].stride;
            ASSERT(stride > 0);

            rows[i].ptr = perm_realloc(rows[i].ptr, stride * len);
            memset(rows[i].ptr + e * stride, 0, stride);
        }
    }
    return e;
}

bool col_rm_h(table_t* table, u32 entName)
{
    i32 e = col_get_h(table, entName);
    if (e != -1)
    {
        i32 b = table->columnct - 1;
        table->columnct = b;

        table->entnames[e] = table->entnames[b];

        const i32 rowct = table->rowct;
        row_t* rows = table->rows;
        for (i32 i = 0; i < rowct; ++i)
        {
            rows[i].length = b;
            i32 stride = rows[i].stride;
            ASSERT(stride > 0);

            memcpy(rows[i].ptr + e * stride, rows[i].ptr + b * stride, stride);
        }

        return true;
    }
    return false;
}
