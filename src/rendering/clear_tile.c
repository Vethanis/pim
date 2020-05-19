#include "rendering/clear_tile.h"
#include "math/float4_funcs.h"
#include "rendering/framebuffer.h"
#include "common/profiler.h"

ProfileMark(pm_ClearTile, ClearTile)
void ClearTile(struct framebuf_s* target, float4 color, float depth)
{
    ProfileBegin(pm_ClearTile);
    ASSERT(target);

    const i32 width = target->width;
    const i32 height = target->height;
    const i32 len = width * height;
    float4* pim_noalias lightBuf = target->light;
    for (i32 i = 0; i < len; ++i)
    {
        lightBuf[i] = color;
    }

    const i32 mipCount = target->mipCount;
    const i32* offsets = target->offsets;
    float* pim_noalias depthBuf = target->depth;
    for (i32 m = 0; m < mipCount; ++m)
    {
        const i32 offset = offsets[m];
        const i32 l = (width >> m) * (height >> m);
        for (i32 i = 0; i < l; ++i)
        {
            depthBuf[offset + i] = depth;
        }
    }

    ProfileEnd(pm_ClearTile);
}
