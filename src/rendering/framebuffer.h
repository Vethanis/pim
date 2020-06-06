#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct framebuf_s
{
    i32 width;
    i32 height;
    float4* light; // linear HDR light buffer (before tonemap, sRGB, dither, and convert)
    float* depth;
    u32* color; // sRGB LDR rgba8 color buffer
} framebuf_t;

void framebuf_create(framebuf_t* buf, i32 width, i32 height);
void framebuf_destroy(framebuf_t* buf);
i32 framebuf_color_bytes(framebuf_t buf);

PIM_C_END
