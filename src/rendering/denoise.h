#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

bool Denoise(
    int2 size,
    const float3* color,
    const float3* albedo,
    const float3* normal,
    float3* output);

PIM_C_END
