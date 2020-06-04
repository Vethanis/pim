#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct vhandle_s
{
    u64 version;
    void* handle;
} vhandle_t;

// copies value into a new allocation
vhandle_t vhandle_new(const void* src, i32 sizeOf);

// releases handle once, returns true if first to release and copies value into dst
bool vhandle_del(vhandle_t hdl, void* dst, i32 sizeOf);

// returns true if handle is alive and copies value into dst
bool vhandle_get(vhandle_t hdl, void* dst, i32 sizeOf);

PIM_C_END
