
#include "math/float4_funcs.h"
#include "rendering/tile.h"
#include "rendering/framebuffer.h"
#include "common/profiler.h"
#include "threading/task.h"
#include "allocator/allocator.h"

typedef struct cleartile_s
{
    task_t task;
    float4 color;
    framebuf_t* target;
    float depth;
} cleartile_t;

static void ClearTileFn(task_t* task, i32 begin, i32 end)
{
    cleartile_t* pSelf = (cleartile_t*)task;
    framebuf_t* target = pSelf->target;
    const float4 color = pSelf->color;
    const float depth = pSelf->depth;
    float4* pim_noalias lightBuf = target->light;
    float* pim_noalias depthBuf = target->depth;

    for (i32 iTile = begin; iTile < end; ++iTile)
    {
        const int2 tile = GetTile(iTile);
        for (i32 ty = 0; ty < kTileHeight; ++ty)
        {
            for (i32 tx = 0; tx < kTileWidth; ++tx)
            {
                i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
                lightBuf[i] = color;
                depthBuf[i] = depth;
            }
        }
    }
}

ProfileMark(pm_ClearTile, ClearTile)
task_t* ClearTile(struct framebuf_s* target, float4 color, float depth)
{
    ProfileBegin(pm_ClearTile);

    ASSERT(target);
    cleartile_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->color = color;
    task->depth = depth;
    task_submit((task_t*)task, ClearTileFn, kTileCount);

    ProfileEnd(pm_ClearTile);
    return (task_t*)task;
}
