#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct DiskBytes_s
{
    i32 offset;
    i32 size;
} DiskBytes;

#define DiskBytes_Check(db, stride) \
    if (db.offset < 0)          { INTERRUPT(); goto cleanup; } \
    if (db.size < 0)            { INTERRUPT(); goto cleanup; } \
    if (db.size % stride)       { INTERRUPT(); goto cleanup; }

pim_inline DiskBytes DiskBytes_New(i32 length, i32 stride, i32* pOffset)
{
    DiskBytes db = { 0 };
    db.size = length * stride;
    db.offset = *pOffset;
    *pOffset = db.offset + db.size;
    return db;
}

PIM_C_END
