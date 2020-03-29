#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct fnd_s { intptr_t handle; } fnd_t;
static int32_t fnd_isopen(fnd_t fdr) { return fdr.handle != -1; }

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
    uint32_t attrib;
    int64_t time_create;
    int64_t time_access;
    int64_t time_write;
    int64_t size;
    char name[260];
} fnd_data_t;

int32_t fnd_errno(void);

int32_t fnd_first(fnd_t* fdr, fnd_data_t* data, const char* spec);
int32_t fnd_next(fnd_t* fdr, fnd_data_t* data);
void fnd_close(fnd_t* fdr);

// Begin + Next + Close
int32_t fnd_iter(fnd_t* fdr, fnd_data_t* data, const char* spec);

PIM_C_END
