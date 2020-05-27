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

typedef struct denoise_s
{
    void* device;
    void* filter;
} denoise_t;

void Denoise_New(denoise_t* denoise);
void Denoise_Del(denoise_t* denoise);

void Denoise_Execute(
    denoise_t* denoise,
    DenoiseType type,
    trace_img_t img,
    float3* output);

PIM_C_END
