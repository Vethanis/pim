#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct FrameBuf_s
{
    i32 width;
    i32 height;
    float4* pim_noalias light; // linear HDR light buffer (before tonemap, sRGB, dither, and convert)
    R8G8B8A8_t* pim_noalias color; // sRGB LDR rgba8 color buffer
} FrameBuf;

void FrameBuf_New(FrameBuf* buf, i32 width, i32 height);
void FrameBuf_Del(FrameBuf* buf);

PIM_C_END
