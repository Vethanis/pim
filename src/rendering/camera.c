#include "rendering/camera.h"
#include "math/frustum.h"
#include "rendering/constants.h"

static camera_t ms_camera = 
{
    .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
    .position = { 0.0f, 0.0f, 5.0f },
    .fovy = 80.0f,
    .nearFar = { 0.1f, 500.0f },
};

void camera_get(camera_t* dst)
{
    ASSERT(dst);
    *dst = ms_camera;
}

void camera_set(const camera_t* src)
{
    ASSERT(src);
    ms_camera = *src;
}

void camera_frustum(const camera_t* src, struct frus_s* dst)
{
    camera_subfrustum(src, dst, f2_s(-1.0f), f2_s(1.0f));
}

void camera_subfrustum(const camera_t* src, struct frus_s* dst, float2 lo, float2 hi)
{
    ASSERT(src);
    ASSERT(dst);

    const float aspect = (float)kDrawWidth / (float)kDrawHeight;
    const float fov = f1_radians(src->fovy);
    const float2 slope = proj_slope(fov, aspect);
    const float4 eye = src->position;
    const quat rot = src->rotation;
    const float2 nearFar = src->nearFar;

    *dst = frus_new(eye, quat_right(rot), quat_up(rot), quat_fwd(rot), lo, hi, slope, nearFar);
}
