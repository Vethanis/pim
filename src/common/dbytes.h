#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct dbytes_s
{
    i32 offset;
    i32 size;
} dbytes_t;

#define dbytes_check(db, stride) \
    if (db.offset < 0)          { INTERRUPT(); goto cleanup; } \
    if (db.size < 0)            { INTERRUPT(); goto cleanup; } \
    if (db.size % stride)       { INTERRUPT(); goto cleanup; }

pim_inline dbytes_t dbytes_new(i32 length, i32 stride, i32* pOffset)
{
    dbytes_t db = { 0 };
    db.size = length * stride;
    db.offset = *pOffset;
    *pOffset = db.offset + db.size;
    return db;
}

PIM_C_END
