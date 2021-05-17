#include "rendering/r_constants.h"
#include "common/cvars.h"
#include "rendering/vulkan/vkr_display.h"

static bool ms_init;

static void EnsureInit(void)
{
    if (!ms_init)
    {
        ms_init = true;
        i32 width = 0;
        i32 height = 0;
        if (vkrDisplay_GetWorkSize(&width, &height));
        {
            ConVar_SetInt(&cv_r_width, width);
            ConVar_SetInt(&cv_r_height, height);
        }
    }
}

i32 r_width_get(void)
{
    EnsureInit();
    return ConVar_GetInt(&cv_r_width);
}

void r_width_set(i32 width)
{
    EnsureInit();
    ConVar_SetInt(&cv_r_width, width);
}

i32 r_height_get(void)
{
    EnsureInit();
    return ConVar_GetInt(&cv_r_height);
}

void r_height_set(i32 height)
{
    EnsureInit();
    ConVar_SetInt(&cv_r_height, height);
}

float r_aspect_get(void)
{
    return (float)r_width_get() / (float)r_height_get();
}

float r_scale_get(void)
{
    EnsureInit();
    return ConVar_GetFloat(&cv_r_scale);
}

void r_scale_set(float scale)
{
    EnsureInit();
    ConVar_SetFloat(&cv_r_scale, scale);
}

i32 r_scaledwidth_get(void)
{
    return (i32)(r_width_get() * r_scale_get() + 0.5f);
}

i32 r_scaledheight_get(void)
{
    return (i32)(r_height_get() * r_scale_get() + 0.5f);
}
