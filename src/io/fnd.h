#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct fnd_s { isize handle; } fnd_t;
static i32 fnd_isopen(fnd_t fdr) { return fdr.handle != -1; }

typedef enum
{
    FAF_Normal = 0x00,
    FAF_ReadOnly = 0x01,
    FAF_Hidden = 0x02,
    FAF_System = 0x04,
    FAF_SubDir = 0x10,
    FAF_Archive = 0x20,
} FileAttrFlags;

// __finddata64_t, do not modify
typedef struct fnd_data_s
{
    u32 attrib;
    i64 time_create;
    i64 time_access;
    i64 time_write;
    i64 size;
    char name[260];
} fnd_data_t;

i32 fnd_errno(void);

i32 fnd_first(fnd_t* fdr, fnd_data_t* data, const char* spec);
i32 fnd_next(fnd_t* fdr, fnd_data_t* data);
void fnd_close(fnd_t* fdr);

// Begin + Next + Close
i32 fnd_iter(fnd_t* fdr, fnd_data_t* data, const char* spec);

PIM_C_END
