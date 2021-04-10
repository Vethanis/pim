#include "rendering/resolve_tile.h"
#include "threading/task.h"
#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "math/color.h"
#include "rendering/tonemap.h"
#include "rendering/vulkan/vkr.h"
#include "common/profiler.h"
#include "allocator/allocator.h"

typedef struct resolve_s
{
    Task task;
    float4 toneParams;
    FrameBuf* target;
    TonemapId tmapId;
} resolve_t;

pim_inline R16G16B16A16_t VEC_CALL Dither(float4 Xi, float4 v)
{
    return f4_rgba16(f4_lerpvs(v, Xi, 1.0f / 65535.0f));
}

static void VEC_CALL ResolvePQ(
    i32 begin, i32 end, FrameBuf* target)
{
    const float Lw = vkrSys_GetWhitepoint();
    const float Lpq = 10000.0f;
    const float wp = Lpq / Lw;
    const float pqRatio = Lw / Lpq;

    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;

    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = tmap4_reinhard_lum(v, wp);
        v = f4_mulvs(v, pqRatio);
        v = f4_PQ_OETF(v);

        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, v);
    }
    Prng_Set(rng);
}

static void VEC_CALL ResolveReinhard(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    float Lw = vkrSys_GetWhitepoint();
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = tmap4_reinhard_lum(light[i], Lw);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_tosrgb(v));
    }
    Prng_Set(rng);
}

static void VEC_CALL ResolveUncharted2(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = tmap4_uchart2(light[i]);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_tosrgb(v));
    }
    Prng_Set(rng);
}

static void VEC_CALL ResolveHable(
    i32 begin, i32 end, FrameBuf* target, float4 params)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = tmap4_hable(light[i], params);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_tosrgb(v));
    }
    Prng_Set(rng);
}

static void VEC_CALL ResolveFilmic(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = tmap4_filmic(light[i]);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_tosrgb(v));
    }
    Prng_Set(rng);
}

static void VEC_CALL ResolveACES(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = tmap4_aces(light[i]);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_tosrgb(v));
    }
    Prng_Set(rng);
}

static void ResolveTileFn(Task* task, i32 begin, i32 end)
{
    resolve_t* resolve = (resolve_t*)task;

    FrameBuf* target = resolve->target;
    const float4 params = resolve->toneParams;
    const TonemapId id = resolve->tmapId;

    if (vkrSys_HdrEnabled())
    {
        ResolvePQ(begin, end, target);
    }
    else
    {
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
        case TMap_ACES:
            ResolveACES(begin, end, target);
            break;
        }
    }

}

ProfileMark(pm_ResolveTile, ResolveTile)
void ResolveTile(FrameBuf* target, TonemapId tmapId, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);
    resolve_t* task = Temp_Calloc(sizeof(*task));
    task->target = target;
    task->tmapId = tmapId;
    task->toneParams = toneParams;
    Task_Run(&task->task, ResolveTileFn, target->width * target->height);

    ProfileEnd(pm_ResolveTile);
}
