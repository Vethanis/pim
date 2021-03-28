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
    FStream file;
    Guid ids[kCrateLen];
    i32 offsets[kCrateLen];
    i32 sizes[kCrateLen];
} Crate;

bool Crate_Open(Crate *const crate, const char* path);
bool Crate_Close(Crate *const crate);

bool Crate_Get(Crate *const crate, Guid id, void* dst, i32 size);
bool Crate_Set(Crate *const crate, Guid id, const void* src, i32 size);
bool Crate_Rm(Crate *const crate, Guid id);

bool Crate_Stat(
    const Crate *const crate,
    Guid id,
    i32 *const offsetOut,
    i32 *const sizeOut);

PIM_C_END
