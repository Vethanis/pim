#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct Camera_s
{
    float4 position;
    quat rotation;
    float zNear;
    float zFar;
    float fovy;
} Camera;

void Camera_Get(Camera* dst);
void Camera_Set(const Camera* src);
void Camera_Reset(void);
void Camera_Frustum(const Camera* src, Frustum* dst, float aspect);
// lo, hi: [-1, 1] range screen bounds
void Camera_SubFrustum(const Camera* src, Frustum* dst, float2 lo, float2 hi, float zNear, float zFar, float aspect);
float4x4 VEC_CALL Camera_GetView(const Camera* src);
float4x4 VEC_CALL Camera_GetProj(const Camera* src, float aspect);
float4x4 VEC_CALL Camera_GetWorldToClip(const Camera* src, float aspect);

PIM_C_END
