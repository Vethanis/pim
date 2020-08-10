#include "rendering/camera.h"
#include "math/frustum.h"
#include "rendering/constants.h"

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
    const quat rot = src->rotation;

    *dst = frus_new(
        src->position,
        quat_right(rot),
        quat_up(rot),
        quat_fwd(rot),
        lo,
        hi,
        proj_slope(fov, aspect),
        src->zNear,
        src->zFar);
}
