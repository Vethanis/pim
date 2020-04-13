#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct camera_s
{
    quat rotation;
    float3 position;
    float fovy;
    float2 nearFar;
} camera_t;

void camera_get(camera_t* dst);
void camera_set(const camera_t* src);

PIM_C_END
