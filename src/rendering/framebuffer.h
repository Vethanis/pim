#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct FrameBuf_s
{
    i32 width;
    i32 height;
    float4* light; // linear HDR light buffer (before tonemap, sRGB, dither, and convert)
    u32* color; // sRGB LDR rgba8 color buffer
} FrameBuf;

void FrameBuf_New(FrameBuf* buf, i32 width, i32 height);
void FrameBuf_Del(FrameBuf* buf);

PIM_C_END
