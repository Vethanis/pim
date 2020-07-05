#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

i32 mipmap_len(int2 size);

float4* mipmap_new_f4(int2 size, EAlloc allocator);
u32* mipmap_new_c32(int2 size, EAlloc allocator);
float* mipmap_new_f32(int2 size, EAlloc allocator);

void mipmap_f4(float4* mipChain, int2 size);
void mipmap_c32(u32* mipChain, int2 size);
void mipmap_f32(float* mipChain, int2 size);

PIM_C_END
