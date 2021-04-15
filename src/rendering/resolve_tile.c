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
    // TODO: debug
    // for some reason the gpu and the path tracer disagree on the whitepoint
    const float wp = vkrSys_GetWhitepoint();
    //const float nits = vkrSys_GetDisplayNits();

    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;

    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = f4_PQ_OOTF(v);
        // This is a bit ridiculous, but it works.
        // multiple-exposure by projecting an hdr band
        // into the sdr range for an sdr tonemapper,
        // then projecting the result back to hdr range.
        float4 a = f4_uncharted2(v, wp);
        float4 b = f4_mulvs(f4_uncharted2(f4_mulvs(v, 0.1f), wp), 10.0f);
        float4 c = f4_mulvs(f4_uncharted2(f4_mulvs(v, 0.01f), wp), 100.0f);
        float4 d = f4_mulvs(f4_uncharted2(f4_mulvs(v, 0.001f), wp), 1000.0f);
        float4 e = f4_mulvs(f4_uncharted2(f4_mulvs(v, 0.0001f), wp), 10000.0f);
        v = f4_mulvs(a, 0.2f);
        v = f4_add(v, f4_mulvs(b, 0.2f));
        v = f4_add(v, f4_mulvs(c, 0.2f));
        v = f4_add(v, f4_mulvs(d, 0.2f));
        v = f4_add(v, f4_mulvs(e, 0.2f));
        v = f4_PQ_InverseEOTF(v);

        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, v);
    }
}

static void VEC_CALL ResolveReinhard(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    float wp = vkrSys_GetWhitepoint();
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = f4_reinhard_lum(light[i], wp);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveUncharted2(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    float wp = vkrSys_GetWhitepoint();
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = f4_uncharted2(light[i], wp);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveHable(
    i32 begin, i32 end, FrameBuf* target, float4 params)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = f4_hable(light[i], params);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveACES(
    i32 begin, i32 end, FrameBuf* target)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = f4_aceskfit(light[i]);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
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
