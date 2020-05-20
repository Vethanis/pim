#pragma once

#include "common/macro.h"
#include "common/fnv1a.h"
#include "containers/idtable.h"

PIM_C_BEGIN

#define TYPE_NAME(T) #T
#define TYPE_ARGS(T) T##_hash, sizeof(T)

typedef struct row_s
{
    u8* ptr;
    i32 stride;
    i32 length;
} row_t;

typedef struct table_s
{
    i32 rowct;          // number of rows per table
    i32 columnct;       // number of columns per row
    u32* ids;           // id for each column / entity
    u32* rownames;      // hashname of each row / type
    row_t* rows;
    idtable_t colidt;
} table_t;

typedef struct tables_s
{
    i32 count;          // number of tables
    u32* hashes;        // hashname of each table
    table_t** tables;   // table pointers
} tables_t;

// ----------------------------------------------------------------------------
// Tables API

// default tables instance
tables_t* tables_main(void);

tables_t* tables_new(void);
void tables_del(tables_t* tables);

table_t* tables_get(tables_t* tables, u32 hash);
bool tables_has(const tables_t* tables, u32 hash);
table_t* tables_add(tables_t* tables, u32 hash);
bool tables_rm(tables_t* tables, u32 hash);

// ----------------------------------------------------------------------------
// Table API

table_t* table_new(void);
void table_del(table_t* table);

row_t* table_get(table_t* table, u32 typeHash);
bool table_has(const table_t* table, u32 typeHash);
bool table_add(table_t* table, u32 typeHash, i32 typeSize);
bool table_rm(table_t* table, u32 typeHash);

// table_get + row_begin
void* table_row(table_t* table, u32 typeHash, i32 typeSize);

static i32 table_width(const table_t* table) { return table->columnct; }
static i32 table_height(const table_t* table) { return table->rowct; }

// ----------------------------------------------------------------------------
// Column API

i32 col_get(const table_t* table, u32 id);
bool col_has(const table_t* table, u32 id);
i32 col_add(table_t* table, u32* idOut);
bool col_rm(table_t* table, u32 id);

// ----------------------------------------------------------------------------
// Row API

static void* row_begin(row_t* row) { return row->ptr; }
static i32 row_len(const row_t* row) { return row->length; }
static i32 row_stride(const row_t* row) { return row->stride; }
void row_set(row_t* row, i32 i, const void* src);
void row_get(const row_t* row, i32 i, void* dst);

// ----------------------------------------------------------------------------
// Lazy String APIs

static table_t* tables_get_s(tables_t* tables, const char* name)
{
    return tables_get(tables, HashStr(name));
}
static bool tables_has_s(const tables_t* tables, const char* name)
{
    return tables_has(tables, HashStr(name));
}
static table_t* tables_add_s(tables_t* tables, const char* name)
{
    return tables_add(tables, HashStr(name));
}
static bool tables_rm_s(tables_t* tables, const char* name)
{
    return tables_rm(tables, HashStr(name));
}

static row_t* table_get_s(table_t* table, const char* typeName)
{
    return table_get(table, HashStr(typeName));
}
static bool table_has_s(const table_t* table, const char* typeName)
{
    return table_has(table, HashStr(typeName));
}
static bool table_add_s(table_t* table, const char* typeName, i32 typeSize)
{
    return table_add(table, HashStr(typeName), typeSize);
}
static bool table_rm_s(table_t* table, const char* typeName)
{
    return table_rm(table, HashStr(typeName));
}

PIM_C_END
