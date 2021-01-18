#pragma once

#include "common/macro.h"
#include "io/fd.h"
#include "io/fstr.h"
#include "io/fmap.h"

PIM_C_BEGIN

typedef enum
{
    vfiletype_null = 0,
    vfiletype_descriptor,
    vfiletype_stream,
    vfiletype_map,
    vfiletype_subfile,
} vfiletype_t;

typedef struct vfilehdl_s
{
    u32 version : 8;
    u32 index : 24;
} vfilehdl_t;

bool vfile_exists(vfilehdl_t hdl);
bool vfile_isopen(vfilehdl_t hdl);
vfilehdl_t vfile_new(const char* path, const char* mode, vfiletype_t type);
bool vfile_del(vfilehdl_t hdl);
i32 vfile_size(vfilehdl_t hdl);
i32 vfile_read(vfilehdl_t hdl, i32 offset, i32 size, void* dst);
i32 vfile_write(vfilehdl_t hdl, i32 offset, i32 size, const void* src);
vfilehdl_t subfile_new(vfilehdl_t owner, i32 offset, i32 size, const char* name);

PIM_C_END
