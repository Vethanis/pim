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

typedef struct TaskResolve_s
{
    Task task;
    float4 toneParams;
    FrameBuf* target;
    TonemapId tmapId;
    float exposure;
    float nits;
    float wp;
} TaskResolve;

pim_inline R16G16B16A16_t VEC_CALL Dither(float4* pim_noalias Xi, float4 v)
{
    *Xi = f4_wrap(f4_add(*Xi, f4_v(kGoldenConj, kSqrt2Conj, kSqrt3Conj, kSqrt5Conj)));
    return f4_rgba16(f4_lerpvs(v, *Xi, 1.0f / 65535.0f));
}

static void VEC_CALL ResolvePQ(
    TaskResolve* pim_noalias task,
    i32 begin, i32 end)
{
    const float4* pim_noalias light = task->target->light;
    R16G16B16A16_t* pim_noalias color = task->target->color;
    // PQ has a 10k range
    const float kToPQNits = 1.0f / 10000.0f;
    // standard / saturation based exposure is calibrated for 100 nits
    const float mp = 100.0f * kToPQNits;
    const float exposure = task->exposure * mp;
    // use display's max nits as whitepoint
    const float wp = task->nits * kToPQNits;

    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToHDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_GtsTonemap(v, wp, mp);
        v = f4_PQ_OETF(v);
        color[i] = Dither(&Xi, v);
    }
}

static void VEC_CALL ResolveReinhard(
    TaskResolve* pim_noalias task,
    i32 begin, i32 end)
{
    const float4* pim_noalias light = task->target->light;
    R16G16B16A16_t* pim_noalias color = task->target->color;
    const float exposure = task->exposure;
    const float wp = task->wp;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_reinhard_lum(v, wp);
        v = f4_sRGB_InverseEOTF_Fit(v);
        color[i] = Dither(&Xi, v);
    }
}

static void VEC_CALL ResolveUncharted2(
    TaskResolve* pim_noalias task,
    i32 begin, i32 end)
{
    const float4* pim_noalias light = task->target->light;
    R16G16B16A16_t* pim_noalias color = task->target->color;
    const float exposure = task->exposure;
    const float wp = task->wp;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_uncharted2(v, wp);
        color[i] = Dither(&Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveHable(
    TaskResolve* pim_noalias task,
    i32 begin, i32 end)
{
    const float4 params = task->toneParams;
    const float4* pim_noalias light = task->target->light;
    R16G16B16A16_t* pim_noalias color = task->target->color;
    const float exposure = task->exposure;
    const float wp = task->wp;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_hable(v, params);
        color[i] = Dither(&Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void VEC_CALL ResolveACES(
    TaskResolve* pim_noalias task,
    i32 begin, i32 end)
{
    const float4* pim_noalias light = task->target->light;
    R16G16B16A16_t* pim_noalias color = task->target->color;
    const float exposure = task->exposure;
    const float wp = task->wp;
    Prng rng = Prng_Get();
    float4 Xi = f4_rand(&rng);
    Prng_Set(rng);
    for (i32 i = begin; i < end; ++i)
    {
        float4 v = light[i];
        v = Color_SceneToSDR(v);
        v = f4_mulvs(v, exposure);
        v = f4_aceskfit(v);
        color[i] = Dither(&Xi, f4_sRGB_InverseEOTF_Fit(v));
    }
}

static void ResolveTileFn(void* pbase, i32 begin, i32 end)
{
    TaskResolve* pim_noalias task = pbase;

    if (vkrSys_HdrEnabled())
    {
        ResolvePQ(task, begin, end);
    }
    else
    {
        switch (task->tmapId)
        {
        default:
        case TMap_Reinhard:
            ResolveReinhard(task, begin, end);
            break;
        case TMap_Uncharted2:
            ResolveUncharted2(task, begin, end);
            break;
        case TMap_Hable:
            ResolveHable(task, begin, end);
            break;
        case TMap_ACES:
            ResolveACES(task, begin, end);
            break;
        }
    }

}

ProfileMark(pm_ResolveTile, ResolveTile)
void ResolveTile(FrameBuf* target, TonemapId tmapId, float4 toneParams)
{
    ProfileBegin(pm_ResolveTile);

    ASSERT(target);

    TaskResolve* task = Temp_Calloc(sizeof(*task));
    task->target = target;
    task->tmapId = tmapId;
    task->toneParams = toneParams;
    task->exposure = vkrExposure_GetParams()->exposure;
    task->nits = vkrSys_GetDisplayNitsMax();
    task->wp = vkrSys_GetWhitepoint();
    Task_Run(&task->task, ResolveTileFn, target->width * target->height);

    ProfileEnd(pm_ResolveTile);
}
