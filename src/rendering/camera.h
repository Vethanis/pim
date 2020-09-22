#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct camera_s
{
    quat rotation;
    float4 position;
    float zNear;
    float zFar;
    float fovy;
} camera_t;

void camera_get(camera_t* dst);
void camera_set(const camera_t* src);
void camera_reset(void);
void camera_frustum(const camera_t* src, frus_t* dst, float aspect);
// lo, hi: [-1, 1] range screen bounds
void camera_subfrustum(const camera_t* src, frus_t* dst, float2 lo, float2 hi, float zNear, float zFar, float aspect); 

PIM_C_END
