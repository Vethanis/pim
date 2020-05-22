#include "rendering/clear_tile.h"
#include "math/float4_funcs.h"
#include "rendering/framebuffer.h"
#include "common/profiler.h"
#include "rendering/sampler.h"
#include "rendering/constants.h"

ProfileMark(pm_ClearTile, ClearTile)
pim_optimize
void ClearTile(struct framebuf_s* target, float4 color, float depth)
{
    ProfileBegin(pm_ClearTile);
    ASSERT(target);

    {
        const i32 len = kDrawWidth * kDrawHeight;
        float4* pim_noalias lightBuf = target->light;
        for (i32 i = 0; i < len; ++i)
        {
            lightBuf[i] = color;
        }
    }

    {
        const int2 size = { kDrawWidth, kDrawHeight };
        const i32 len = 1 + CalcMipOffset(size, CalcMipCount(size));
        float* pim_noalias depthBuf = target->depth;
        for (i32 i = 0; i < len; ++i)
        {
            depthBuf[i] = depth;
        }
    }

    ProfileEnd(pm_ClearTile);
}
