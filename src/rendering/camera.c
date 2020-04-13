#include "rendering/camera.h"

static camera_t ms_camera = 
{
    .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
    .position = { 0.0f, 0.0f, 0.0f },
    .fovy = 90.0f,
    .nearFar = { 0.05f, 200.0f },
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
