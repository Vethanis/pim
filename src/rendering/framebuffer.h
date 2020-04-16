#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct framebuf_s
{
    i32 width;
    i32 height;
    u32* color; // rgba8
    float* depth;
} framebuf_t;

void framebuf_create(framebuf_t* buf, i32 width, i32 height);
void framebuf_destroy(framebuf_t* buf);

i32 framebuf_color_bytes(framebuf_t buf);
i32 framebuf_depth_bytes(framebuf_t buf);

void framebuf_clear(framebuf_t buf, u32 color, float depth);

PIM_C_END
