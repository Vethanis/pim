#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/path_tracer.h"

PIM_C_BEGIN

typedef enum
{
    DenoiseType_Image,
    DenoiseType_Lightmap,

    DenoiseType_COUNT
} DenoiseType;

void Denoise_Execute(
    DenoiseType type,
    trace_img_t img,
    float3* output);

PIM_C_END
