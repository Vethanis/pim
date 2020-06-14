#include "rendering/resolve_tile.h"
#include "threading/task.h"
#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
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

pim_optimize
static void ResolveTileFn(task_t* task, i32 begin, i32 end)
{
    resolve_t* resolve = (resolve_t*)task;

    framebuf_t* target = resolve->target;
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;

    const float4 tmapParams = resolve->toneParams;
    const TonemapFn tmap = resolve->tonemapper;

    const float kDitherScale = 1.0f / 255.0f;
    prng_t rng = prng_get();

    for (i32 iTile = begin; iTile < end; ++iTile)
    {
        const int2 tile = GetTile(iTile);
        for (i32 ty = 0; ty < kTileHeight; ++ty)
        {
            for (i32 tx = 0; tx < kTileWidth; ++tx)
            {
                i32 i = IndexTile(tile, kDrawWidth, tx, ty);
                float4 ldr = tmap(light[i], tmapParams);
                float4 srgb = f4_tosrgb(ldr);
                srgb = f4_lerpvs(srgb, f4_rand(&rng), kDitherScale);
                color[i] = f4_rgba8(srgb);
            }
        }
    }

    prng_set(rng);
}

ProfileMark(pm_ResolveTile, ResolveTile)
void ResolveTile(framebuf_t* target, TonemapId tonemapper, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);
    resolve_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->tonemapper = Tonemap_GetFunc(tonemapper);
    task->toneParams = toneParams;
    task_run(&task->task, ResolveTileFn, kTileCount);

    ProfileEnd(pm_ResolveTile);
}
