#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct Camera_s
{
    quat rotation;
    float4 position;
    float zNear;
    float zFar;
    float fovy;
} Camera;

void camera_get(Camera* dst);
void camera_set(const Camera* src);
void camera_reset(void);
void camera_frustum(const Camera* src, Frustum* dst, float aspect);
// lo, hi: [-1, 1] range screen bounds
void camera_subfrustum(const Camera* src, Frustum* dst, float2 lo, float2 hi, float zNear, float zFar, float aspect);

PIM_C_END
