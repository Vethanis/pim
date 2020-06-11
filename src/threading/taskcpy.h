#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

void taskcpy(void* dst, const void* src, i32 sizeOf, i32 length);

void blit_3to4(int2 size, float4* dst, const float3* src);
void blit_4to3(int2 size, float3* dst, const float4* src);

float4* blitnew_3to4(int2 size, const float3* src, EAlloc allocator);
float3* blitnew_4to3(int2 size, const float4* src, EAlloc allocator);

PIM_C_END
