#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct framebuf_s
{
    i32 width;
    i32 height;
    float4* light; // HDR lighting written here. later pass converts it to color
    float* depth;
    u32* color; // rgba8
    u32* tileFlags;
} framebuf_t;

void framebuf_create(framebuf_t* buf, i32 width, i32 height);
void framebuf_destroy(framebuf_t* buf);

i32 framebuf_color_bytes(framebuf_t buf);
i32 framebuf_depth_bytes(framebuf_t buf);

PIM_C_END
