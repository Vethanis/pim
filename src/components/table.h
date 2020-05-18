#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "common/fnv1a.h"

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
    u32* entnames;      // hashname for each column / entity
    u32* rownames;      // hashname of each row / type
    row_t* rows;
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

table_t* tables_get_h(tables_t* tables, u32 hash);
bool tables_has_h(const tables_t* tables, u32 hash);
table_t* tables_add_h(tables_t* tables, u32 hash);
bool tables_rm_h(tables_t* tables, u32 hash);

// ----------------------------------------------------------------------------
// Table API

table_t* table_new(void);
void table_del(table_t* table);

row_t* table_get_h(table_t* table, u32 typeHash);
bool table_has_h(const table_t* table, u32 typeHash);
bool table_add_h(table_t* table, u32 typeHash, i32 typeSize);
bool table_rm_h(table_t* table, u32 typeHash);

// ----------------------------------------------------------------------------
// Column API

i32 col_get_h(const table_t* table, u32 entName);
bool col_has_h(const table_t* table, u32 entName);
i32 col_add_h(table_t* table, u32 entName);
bool col_rm_h(table_t* table, u32 entName);

// ----------------------------------------------------------------------------
// Row API

static void* row_begin(row_t* row) { return row->ptr; }
static i32 row_len(const row_t* row) { return row->length; }
static i32 row_stride(const row_t* row) { return row->stride; }

// ----------------------------------------------------------------------------
// Lazy String APIs

static table_t* tables_get_s(tables_t* tables, const char* name)
{
    return tables_get_h(tables, HashStr(name));
}
static bool tables_has_s(const tables_t* tables, const char* name)
{
    return tables_has_h(tables, HashStr(name));
}
static table_t* tables_add_s(tables_t* tables, const char* name)
{
    return tables_add_h(tables, HashStr(name));
}
static bool tables_rm_s(tables_t* tables, const char* name)
{
    return tables_rm_h(tables, HashStr(name));
}

static row_t* table_get_s(table_t* table, const char* typeName)
{
    return table_get_h(table, HashStr(typeName));
}
static bool table_has_s(const table_t* table, const char* typeName)
{
    return table_has_h(table, HashStr(typeName));
}
static bool table_add_s(table_t* table, const char* typeName, i32 typeSize)
{
    return table_add_h(table, HashStr(typeName), typeSize);
}
static bool table_rm_s(table_t* table, const char* typeName)
{
    return table_rm_h(table, HashStr(typeName));
}

static i32 col_get_s(const table_t* table, const char* entName)
{
    return col_get_h(table, HashStr(entName));
}
static bool col_has_s(const table_t* table, const char* entName)
{
    return col_has_h(table, HashStr(entName));
}
static bool col_add_s(table_t* table, const char* entName)
{
    return col_add_h(table, HashStr(entName));
}
static bool col_rm_s(table_t* table, const char* entName)
{
    return col_rm_h(table, HashStr(entName));
}

PIM_C_END
