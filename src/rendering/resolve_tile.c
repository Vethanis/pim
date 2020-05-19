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

pim_optimize
static void ResolveTileFn(task_t* task, i32 begin, i32 end)
{
    resolve_t* resolve = (resolve_t*)task;
    framebuf_t* target = resolve->target;
    const float4 tmapParams = resolve->toneParams;
    const TonemapFn tmap = resolve->tonemapper;
    const i32 mipCount = target->mipCount;
    const i32* offsets = target->offsets;
    float* pim_noalias depth = target->depth;
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;

    const float kDitherScale = 1.0f / 256.0f;
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
                ldr = f4_lerpvs(ldr, f4_rand(&rng), kDitherScale);
                color[i] = f4_rgba8(ldr);
            }
        }
    }
    prng_set(rng);
}

ProfileMark(pm_ResolveTile, ResolveTile)
pim_optimize
struct task_s* ResolveTile(struct framebuf_s* target, TonemapId tonemapper, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);
    resolve_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->tonemapper = Tonemap_GetFunc(tonemapper);
    task->toneParams = toneParams;
    task_submit((task_t*)task, ResolveTileFn, kTileCount);
    task_sys_schedule();

    // DONT BUGS
    // OPEN INSIDE
    float* pim_noalias depth = target->depth;
    const i32 mipCount = target->mipCount;
    const i32* offsets = target->offsets;
    for (i32 m = 0; (m + 1) < mipCount; ++m)
    {
        const i32 n = m + 1;

        const i32 mw = target->width >> m;
        const i32 mh = target->height >> m;
        const i32 nw = target->width >> n;
        const i32 nh = target->height >> n;

        const i32 m0 = offsets[m];
        const i32 n0 = offsets[n];
        const i32 n1 = n0 + (nw*nh);

        for (i32 y = 0; (y + 2) < mh; y += 2)
        {
            for (i32 x = 0; (x + 2) < mw; x += 2)
            {
                i32 a = m0 + (x + 0) + (y + 0) * mw;
                i32 b = m0 + (x + 1) + (y + 0) * mw;
                i32 c = m0 + (x + 0) + (y + 1) * mw;
                i32 d = m0 + (x + 1) + (y + 1) * mw;

                i32 e = n0 + ((x >> 1) + (y >> 1) * nw);

                ASSERT(a < n0);
                ASSERT(a >= m0);
                ASSERT(b < n0);
                ASSERT(b >= m0);
                ASSERT(c < n0);
                ASSERT(c > m0);
                ASSERT(d < n0);
                ASSERT(d >= m0);
                ASSERT(e < n1);
                ASSERT(e >= n0);

                float da = depth[a];
                float db = depth[b];
                float dc = depth[c];
                float dd = depth[d];
                depth[e] = f1_max(f1_max(da, db), f1_max(dc, dd));
            }
        }
    }

    ProfileEnd(pm_ResolveTile);
    return (task_t*)task;
}
