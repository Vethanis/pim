#include "rendering/camera.h"
#include "math/frustum.h"

static camera_t ms_camera = 
{
    .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
    .position = { 0.0f, 0.0f, 5.0f },
    .zNear = 0.1f,
    .zFar = 500.0f,
    .fovy = 90.0f,
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

void camera_reset(void)
{
    ms_camera.position = f4_0;
    ms_camera.rotation = quat_id;
}

void camera_frustum(const camera_t* src, frus_t* dst, float aspect)
{
    camera_subfrustum(src, dst, f2_s(-1.0f), f2_s(1.0f), src->zNear, src->zFar, aspect);
}

void camera_subfrustum(const camera_t* src, frus_t* dst, float2 lo, float2 hi, float zNear, float zFar, float aspect)
{
    ASSERT(src);
    ASSERT(dst);

    const float fov = f1_radians(src->fovy);
    const quat rot = src->rotation;

    *dst = frus_new(
        src->position,
        quat_right(rot),
        quat_up(rot),
        quat_fwd(rot),
        lo,
        hi,
        proj_slope(fov, aspect),
        zNear,
        zFar);
}
