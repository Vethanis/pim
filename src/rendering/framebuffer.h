#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct framebuf_s
{
    int32_t width;
    int32_t height;
    uint16_t* color;
    uint16_t* depth;
} framebuf_t;

void framebuf_create(framebuf_t* buf, int32_t width, int32_t height);
void framebuf_destroy(framebuf_t* buf);

int32_t framebuf_color_bytes(framebuf_t buf);
int32_t framebuf_depth_bytes(framebuf_t buf);

void framebuf_clear(framebuf_t buf, float color[4], float depth);
int32_t framebuf_dtest_lte(framebuf_t buf, int32_t x, int32_t y, float z);
void framebuf_wcolor(framebuf_t buf, int32_t x, int32_t y, float color[4]);

void f4_dither(float x[4], float amt);

uint32_t f4_to_rgba8(float x[4]);
uint16_t f4_to_rgba4(float x[4]);
uint16_t f4_to_rgb5a1(float x[4]);
uint16_t f1_to_r16(float x);

PIM_C_END
