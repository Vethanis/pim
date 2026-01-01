#include "rendering/camera.h"
#include "math/float4x4_funcs.h"
#include "math/frustum.h"

static Camera ms_camera = 
{
    .position = { 0.0f, 0.0f, 5.0f, 1.0f },
    .rotation = { { 0.0f, 0.0f, 0.0f, 1.0f } },
    .zNear = 0.1f,
    .zFar = 500.0f,
    .fovy = 90.0f,
};

void Camera_Get(Camera* dst)
{
    ASSERT(dst);
    *dst = ms_camera;
}

void Camera_Set(const Camera* src)
{
    ASSERT(src);
    ms_camera = *src;
}

void Camera_Reset(void)
{
    ms_camera.position = f4_0;
    ms_camera.rotation = quat_id;
}

void Camera_Frustum(const Camera* src, Frustum* dst, float aspect)
{
    Camera_SubFrustum(
        src, dst,
        f2_s(-1.0f), f2_s(1.0f),
        src->zNear, src->zFar,
        aspect);
}

void Camera_SubFrustum(
    const Camera* src,
    Frustum* dst,
    float2 lo, float2 hi,
    float zNear, float zFar,
    float aspect)
{
    ASSERT(src);
    ASSERT(dst);
    quat rot = src->rotation;
    *dst = frus_new(
        src->position,
        quat_right(rot),
        quat_up(rot),
        quat_fwd(rot),
        lo,
        hi,
        proj_slope(f1_radians(src->fovy), aspect),
        zNear,
        zFar);
}

float4x4 VEC_CALL Camera_GetView(const Camera* src)
{
    float4 pos = src->position;
    quat rot = src->rotation;
    return f4x4_lookat(
        pos,
        f4_add(pos, quat_fwd(rot)),
        quat_up(rot));
}

float4x4 VEC_CALL Camera_GetProj(const Camera* src, float aspect)
{
    return f4x4_vkperspective(
        f1_radians(src->fovy),
        aspect,
        src->zNear, src->zFar);
}

float4x4 VEC_CALL Camera_GetWorldToClip(const Camera* src, float aspect)
{
    return f4x4_mul(
        Camera_GetProj(src, aspect),
        Camera_GetView(src));
}
