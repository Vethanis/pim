#pragma once

#include "common/macro.h"
#include "common/guid.h"
#include "io/fstr.h"

PIM_C_BEGIN

enum
{
    kCrateLen = 1024,
};

typedef struct Crate_s
{
    fstr_t file;
    Guid ids[kCrateLen];
    i32 offsets[kCrateLen];
    i32 sizes[kCrateLen];
} Crate;

bool crate_open(Crate *const crate, const char* path);
bool crate_close(Crate *const crate);

bool crate_get(Crate *const crate, Guid id, void* dst, i32 size);
bool crate_set(Crate *const crate, Guid id, const void* src, i32 size);
bool crate_rm(Crate *const crate, Guid id);

bool crate_stat(
    const Crate *const crate,
    Guid id,
    i32 *const offsetOut,
    i32 *const sizeOut);

PIM_C_END
