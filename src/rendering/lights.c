#include "rendering/lights.h"
#include "allocator/allocator.h"

static lights_t ms_lights;

lights_t* lights_get(void)
{
    return &ms_lights;
}

void lights_clear(void)
{
    ms_lights.ptCount = 0;
    ms_lights.dirCount = 0;
}

i32 lights_add_pt(pt_light_t pt)
{
    i32 len = ++ms_lights.ptCount;
    ms_lights.ptLights = perm_realloc(ms_lights.ptLights, sizeof(pt) * len);
    ms_lights.ptLights[len - 1] = pt;
    return len - 1;
}

i32 lights_add_dir(dir_light_t dir)
{
    i32 len = ++ms_lights.dirCount;
    ms_lights.dirLights = perm_realloc(ms_lights.ptLights, sizeof(dir) * len);
    ms_lights.dirLights[len - 1] = dir;
    return len - 1;
}

void lights_rm_pt(i32 i)
{
    i32 len = ms_lights.ptCount;
    pt_light_t* pts = ms_lights.ptLights;
    if (i >= 0 && i < len)
    {
        for (i32 j = i + 1; j < len; ++j)
        {
            pts[j - 1] = pts[j];
        }
        ms_lights.ptCount = len - 1;
    }
}

void lights_rm_dir(i32 i)
{
    i32 len = ms_lights.dirCount;
    dir_light_t* dirs = ms_lights.dirLights;
    if (i >= 0 && i < len)
    {
        for (i32 j = i + 1; j < len; ++j)
        {
            dirs[j - 1] = dirs[j];
        }
        ms_lights.dirCount = len - 1;
    }
}

void lights_set_pt(i32 i, pt_light_t pt)
{
    ASSERT(i >= 0 && i < ms_lights.ptCount);
    ms_lights.ptLights[i] = pt;
}

void lights_set_dir(i32 i, dir_light_t dir)
{
    ASSERT(i >= 0 && i < ms_lights.dirCount);
    ms_lights.dirLights[i] = dir;
}

pt_light_t lights_get_pt(i32 i)
{
    ASSERT(i >= 0 && i < ms_lights.ptCount);
    return ms_lights.ptLights[i];
}

dir_light_t lights_get_dir(i32 i)
{
    ASSERT(i >= 0 && i < ms_lights.dirCount);
    return ms_lights.dirLights[i];
}

i32 lights_pt_count(void)
{
    return ms_lights.ptCount;
}

i32 lights_dir_count(void)
{
    return ms_lights.dirCount;
}
