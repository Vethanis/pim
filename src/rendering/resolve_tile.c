#include "rendering/resolve_tile.h"
#include "threading/task.h"
#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "rendering/tonemap.h"
#include "common/profiler.h"
#include "allocator/allocator.h"

typedef struct resolve_s
{
    task_t task;
    float4 toneParams;
    framebuf_t* target;
    TonemapId tmapId;
} resolve_t;

pim_inline u32 VEC_CALL ToColor(prng_t* rng, float4 linear)
{
    const float4 kWeight = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 0.0f };
    float4 srgb = f4_tosrgb(linear);
    srgb = f4_lerp(srgb, f4_rand(rng), kWeight);
    u32 color = f4_rgba8(srgb);
    return color;
}

static void VEC_CALL ResolveReinhard(
    i32 begin, i32 end, framebuf_t* target)
{
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;
    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        color[i] = ToColor(&rng, tmap4_reinhard(light[i]));
    }
    prng_set(rng);
}

static void VEC_CALL ResolveUncharted2(
    i32 begin, i32 end, framebuf_t* target)
{
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;
    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        float4 hdr = light[i];
        hdr.w = 1.0f;
        color[i] = ToColor(&rng, tmap4_uchart2(hdr));
    }
    prng_set(rng);
}

static void VEC_CALL ResolveHable(
    i32 begin, i32 end, framebuf_t* target, float4 params)
{
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;
    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        float4 hdr = light[i];
        hdr.w = 1.0f;
        color[i] = ToColor(&rng, tmap4_hable(hdr, params));
    }
    prng_set(rng);
}

static void VEC_CALL ResolveFilmic(
    i32 begin, i32 end, framebuf_t* target)
{
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;
    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        color[i] = ToColor(&rng, tmap4_filmic(light[i]));
    }
    prng_set(rng);
}

static void VEC_CALL ResolveACES(
    i32 begin, i32 end, framebuf_t* target)
{
    float4* pim_noalias light = target->light;
    u32* pim_noalias color = target->color;
    prng_t rng = prng_get();
    for (i32 i = begin; i < end; ++i)
    {
        color[i] = ToColor(&rng, tmap4_aces(light[i]));
    }
    prng_set(rng);
}

pim_optimize
static void ResolveTileFn(task_t* task, i32 begin, i32 end)
{
    resolve_t* resolve = (resolve_t*)task;

    framebuf_t* target = resolve->target;
    const float4 params = resolve->toneParams;
    const TonemapId id = resolve->tmapId;

    switch (id)
    {
    default:
    case TMap_Reinhard:
        ResolveReinhard(begin, end, target);
        break;
    case TMap_Uncharted2:
        ResolveUncharted2(begin, end, target);
        break;
    case TMap_Hable:
        ResolveHable(begin, end, target, params);
        break;
    case TMap_Filmic:
        ResolveFilmic(begin, end, target);
        break;
    case TMap_ACES:
        ResolveACES(begin, end, target);
        break;
    }
}

ProfileMark(pm_ResolveTile, ResolveTile)
void ResolveTile(framebuf_t* target, TonemapId tmapId, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);
    resolve_t* task = tmp_calloc(sizeof(*task));
    task->target = target;
    task->tmapId = tmapId;
    task->toneParams = toneParams;
    task_run(&task->task, ResolveTileFn, target->width * target->height);

    ProfileEnd(pm_ResolveTile);
}
