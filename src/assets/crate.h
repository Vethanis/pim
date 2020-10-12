#pragma once

#include "common/macro.h"
#include "common/guid.h"
#include "io/fstr.h"

PIM_C_BEGIN

enum
{
    kCrateLen = 1024,
};

typedef struct crate_s
{
    fstr_t file;
    guid_t ids[kCrateLen];
    i32 offsets[kCrateLen];
    i32 sizes[kCrateLen];
} crate_t;

bool crate_open(crate_t* crate, const char* path);
bool crate_close(crate_t* crate);

bool crate_get(crate_t* crate, guid_t id, void* dst, i32 size);
bool crate_set(crate_t* crate, guid_t id, const void* src, i32 size);
bool crate_rm(crate_t* crate, guid_t id);

bool crate_stat(const crate_t* crate, guid_t id, i32* offsetOut, i32* sizeOut);

PIM_C_END
