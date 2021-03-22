#include "rendering/lights.h"
#include "allocator/allocator.h"

static Lights ms_lights;

Lights* Lights_Get(void)
{
    return &ms_lights;
}

void Lights_Clear(void)
{
    ms_lights.ptCount = 0;
    ms_lights.dirCount = 0;
}

i32 Lights_AddPt(PtLight pt)
{
    i32 len = ++ms_lights.ptCount;
    ms_lights.ptLights = Perm_Realloc(ms_lights.ptLights, sizeof(pt) * len);
    ms_lights.ptLights[len - 1] = pt;
    return len - 1;
}

i32 Lights_AddDir(DirLight dir)
{
    i32 len = ++ms_lights.dirCount;
    ms_lights.dirLights = Perm_Realloc(ms_lights.ptLights, sizeof(dir) * len);
    ms_lights.dirLights[len - 1] = dir;
    return len - 1;
}

void Lights_RmPt(i32 i)
{
    i32 len = ms_lights.ptCount;
    PtLight* pts = ms_lights.ptLights;
    if (i >= 0 && i < len)
    {
        for (i32 j = i + 1; j < len; ++j)
        {
            pts[j - 1] = pts[j];
        }
        ms_lights.ptCount = len - 1;
    }
}

void Lights_RmDir(i32 i)
{
    i32 len = ms_lights.dirCount;
    DirLight* dirs = ms_lights.dirLights;
    if (i >= 0 && i < len)
    {
        for (i32 j = i + 1; j < len; ++j)
        {
            dirs[j - 1] = dirs[j];
        }
        ms_lights.dirCount = len - 1;
    }
}

void Lights_SetPt(i32 i, PtLight pt)
{
    ASSERT(i >= 0 && i < ms_lights.ptCount);
    ms_lights.ptLights[i] = pt;
}

void Lights_SetDir(i32 i, DirLight dir)
{
    ASSERT(i >= 0 && i < ms_lights.dirCount);
    ms_lights.dirLights[i] = dir;
}

PtLight Lights_GetPt(i32 i)
{
    ASSERT(i >= 0 && i < ms_lights.ptCount);
    return ms_lights.ptLights[i];
}

DirLight Lights_GetDir(i32 i)
{
    ASSERT(i >= 0 && i < ms_lights.dirCount);
    return ms_lights.dirLights[i];
}

i32 Lights_PtCount(void)
{
    return ms_lights.ptCount;
}

i32 Lights_DirCount(void)
{
    return ms_lights.dirCount;
}
