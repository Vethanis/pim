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
void camera_frustum(const camera_t* src, struct frus_s* dst);
void camera_subfrustum(const camera_t* src, struct frus_s* dst, float2 lo, float2 hi); // lo, hi: [-1, 1] range screen bounds

PIM_C_END
