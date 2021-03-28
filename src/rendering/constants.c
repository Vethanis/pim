#include "rendering/constants.h"
#include "common/cvar.h"
#include "rendering/vulkan/vkr_display.h"

static bool ms_init;

static ConVar_t cv_r_scale =
{
    .type = cvart_float,
    .name = "r_scale",
    .minFloat = 1.0f / 16.0f,
    .maxFloat = 4.0f,
#if _DEBUG
    .value = "0.1",
#else
    .value = "0.5",
#endif // _DEBUG
    .desc = "Render Scale",
};

static ConVar_t cv_r_width =
{
    .type = cvart_int,
    .name = "r_width",
    .minInt = 1,
    .maxInt = 8192,
    .value = "1920",
    .desc = "Base render width",
};

static ConVar_t cv_r_height =
{
    .type = cvart_int,
    .name = "r_height",
    .minInt = 1,
    .maxInt = 8192,
    .value = "1070",
    .desc = "Base render height",
};

static void EnsureInit(void)
{
    if (!ms_init)
    {
        ms_init = true;
        ConVar_Reg(&cv_r_scale);
        ConVar_Reg(&cv_r_width);
        ConVar_Reg(&cv_r_height);
        i32 width = 0;
        i32 height = 0;
        if (vkrDisplay_MonitorSize(&width, &height));
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
