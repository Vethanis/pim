#include "rendering/resolve_tile.h"
#include "threading/task.h"
#include "rendering/framebuffer.h"
#include "math/color.h"
#include "rendering/constants.h"
#include "rendering/tonemap.h"
#include "rendering/tile.h"
#include "common/profiler.h"
#include "allocator/allocator.h"

typedef struct resolve_s
{
    task_t task;
    float4 toneParams;
    framebuf_t* target;
    TonemapFn tonemapper;
} resolve_t;

ProfileMark(pm_ResolveTileFn, ResolveTileFn)
static void ResolveTileFn(task_t* task, i32 begin, i32 end)
{
    ProfileBegin(pm_ResolveTileFn);

    resolve_t* resolve = (resolve_t*)task;
    framebuf_t* target = resolve->target;
    const float4 tmapParams = resolve->toneParams;
    const TonemapFn tmap = resolve->tonemapper;
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;

    for (i32 iTile = begin; iTile < end; ++iTile)
    {
        AcquireTile(target, iTile);
        const int2 tile = GetTile(iTile);
        for (i32 ty = 0; ty < kTileHeight; ++ty)
        {
            for (i32 tx = 0; tx < kTileWidth; ++tx)
            {
                i32 i = (tile.x + tx) + (tile.y + ty) * kDrawWidth;
                color[i] = f4_rgba8(tmap(light[i], tmapParams));
            }
        }
        ReleaseTile(target, iTile);
    }

    ProfileEnd(pm_ResolveTileFn);
}

ProfileMark(pm_ResolveTile, ResolveTile)
struct task_s* ResolveTile(struct framebuf_s* target, TonemapId tonemapper, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);
    resolve_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->tonemapper = Tonemap_GetFunc(tonemapper);
    task->toneParams = toneParams;
    task_submit((task_t*)task, ResolveTileFn, kTileCount);

    ProfileEnd(pm_ResolveTile);
    return (task_t*)task;
}
