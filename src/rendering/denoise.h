#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef enum
{
    DenoiseType_Image,
    DenoiseType_Lightmap,

    DenoiseType_COUNT
} DenoiseType;

bool Denoise(
    DenoiseType type,
    int2 size,
    const float3* color,
    const float3* albedo,
    const float3* normal,
    float3* output);

void Denoise_Evict(void);

PIM_C_END
