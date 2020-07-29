#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct camera_s
{
    quat rotation;
    float4 position;
    float2 nearFar;
    float fovy;
} camera_t;
static const u32 camera_t_hash = 2034616629u;

void camera_get(camera_t* dst);
void camera_set(const camera_t* src);
void camera_reset(void);
void camera_frustum(const camera_t* src, struct frus_s* dst);
void camera_subfrustum(const camera_t* src, struct frus_s* dst, float2 lo, float2 hi); // lo, hi: [-1, 1] range screen bounds

PIM_C_END
