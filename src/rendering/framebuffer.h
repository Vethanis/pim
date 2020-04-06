#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct framebuf_s
{
    int32_t width;
    int32_t height;
    uint16_t* color; // rgb5a1
    uint16_t* depth; // z16
} framebuf_t;

void framebuf_create(framebuf_t* buf, int32_t width, int32_t height);
void framebuf_destroy(framebuf_t* buf);

int32_t framebuf_color_bytes(framebuf_t buf);
int32_t framebuf_depth_bytes(framebuf_t buf);

void framebuf_clear(framebuf_t buf, uint16_t color, uint16_t depth);

PIM_C_END
