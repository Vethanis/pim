#include "rendering/resolve_tile.h"

#include "rendering/framebuffer.h"
#include "rendering/sampler.h"
#include "rendering/tonemap.h"
#include "rendering/vulkan/vkr.h"
#include "rendering/vulkan/vkr_exposure.h"

#include "allocator/allocator.h"
#include "threading/task.h"
#include "common/profiler.h"
#include "common/cvars.h"
#include "math/color.h"

typedef struct resolve_s
{
    Task task;
    float4 toneParams;
    FrameBuf* target;
    TonemapId tmapId;
    float exposure;
    float wp;
} resolve_t;

pim_inline R16G16B16A16_t VEC_CALL Dither(float4 Xi, float4 v)
{
    return f4_rgba16(f4_lerpvs(v, Xi, 1.0f / 65535.0f));
}

static void VEC_CALL ResolvePQ(
    i32 begin, i32 end, FrameBuf* target, float exposure, float wp)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToHDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_PQ_OOTF(v);
        v = f4_PQ_InverseEOTF(v);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, v);
    }
}

static void VEC_CALL ResolveReinhard(
    i32 begin, i32 end, FrameBuf* target, float exposure, float wp)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_reinhard_lum(v, wp);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveUncharted2(
    i32 begin, i32 end, FrameBuf* target, float exposure, float wp)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_uncharted2(v, wp);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveHable(
    i32 begin, i32 end, FrameBuf* target, float4 params, float exposure, float wp)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_hable(v, params);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveACES(
    i32 begin, i32 end, FrameBuf* target, float exposure, float wp)
{
    float4* pim_noalias light = target->light;
    R16G16B16A16_t* pim_noalias color = target->color;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_aceskfit(v);
        Xi = f4_wrap(f4_add(Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
        color[i] = Dither(Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void ResolveTileFn(void* pbase, i32 begin, i32 end)
{
    resolve_t* task = pbase;

    FrameBuf* target = task->target;
    const float4 params = task->toneParams;
    const TonemapId id = task->tmapId;
    const float exposure = task->exposure;
    const float wp = task->wp;

    if (vkrSys_HdrEnabled())
    {
        ResolvePQ(begin, end, target, exposure, wp);
    }
    else
    {
        switch (id)
        {
        default:
        case TMap_Reinhard:
            ResolveReinhard(begin, end, target, exposure, wp);
            break;
        case TMap_Uncharted2:
            ResolveUncharted2(begin, end, target, exposure, wp);
            break;
        case TMap_Hable:
            ResolveHable(begin, end, target, params, exposure, wp);
            break;
        case TMap_ACES:
            ResolveACES(begin, end, target, exposure, wp);
            break;
        }
    }

}

ProfileMark(pm_ResolveTile, ResolveTile)
void ResolveTile(FrameBuf* target, TonemapId tmapId, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);

    const float wp = vkrSys_GetWhitepoint();
    const float nits = vkrSys_GetDisplayNitsMax();
    float exposure = vkrExposure_GetParams()->exposure;
    if (vkrSys_HdrEnabled())
    {
        exposure *= Color_PQExposure(nits, wp);
    }

    resolve_t* task = Temp_Calloc(sizeof(*task));
    task->target = target;
    task->tmapId = tmapId;
    task->toneParams = toneParams;
    task->exposure = exposure;
    task->wp = wp;
    Task_Run(&task->task, ResolveTileFn, target->width * target->height);

    ProfileEnd(pm_ResolveTile);
}
