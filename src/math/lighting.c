#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/cubemap.h"
#include <string.h>

// [Karis13]
static float2 VEC_CALL IntegrateBRDF(
    float cosTheta,
    float roughness,
    u32 numSamples)
{
    float2 result = { 0.0f, 0.0f };
    const float4 V = SphericalToCartesian(cosTheta, 0.0f);
    const float4 I = f4_neg(V);
    const float alpha = BrdfAlpha(roughness);
    const float weight = 1.0f / numSamples;
    for (u32 i = 0; i < numSamples; ++i)
    {
        float4 m = SampleGGXMicrofacet(Hammersley2D(i, numSamples), alpha);
        float4 L = f4_reflect3(I, m);
        float NoL = f1_sat(L.z);
        float NoH = f1_sat(m.z);
        float HoV = f4_dotsat(m, V);
        float pdf = GGXPdf(NoH, HoV, alpha);
        if ((NoL > 0.0f) && (pdf > 0.0f))
        {
            float D = D_GTR(NoH, alpha) / pdf;
            float G = G_SmithGGX(NoL, cosTheta, alpha);
            float term = D * G * NoL;
            float F = F_Schlick1(0.0f, 1.0f, HoV);
            result.x += weight * ((1.0f - F) * term);
            result.y += weight * (F * term);
        }
    }
    return result;
}

typedef struct BakeBrdfTask
{
    Task task;
    BrdfLut lut;
    u32 numSamples;
} BakeBrdfTask;

static void BakeBrdfFn(void* pbase, i32 begin, i32 end)
{
    BakeBrdfTask* pim_noalias task = pbase;
    float2* pim_noalias texels = task->lut.texels;
    int2 size = task->lut.size;
    u32 numSamples = task->numSamples;
    for (i32 i = begin; i < end; ++i)
    {
        float2 uv = IndexToUv(size, i);
        float cosTheta = uv.x;
        float roughness = uv.y;
        texels[i] = IntegrateBRDF(cosTheta, roughness, numSamples);
    }
}

BrdfLut BrdfLut_New(int2 size, u32 numSamples)
{
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);

    BakeBrdfTask* task = Temp_Calloc(sizeof(*task));
    task->lut.texels = Tex_Alloc(sizeof(task->lut.texels[0]) * size.x * size.y);
    task->lut.size = size;
    task->numSamples = numSamples;
    Task_Run(task, BakeBrdfFn, size.x * size.y);

    return task->lut;
}

void FreeBrdfLut(BrdfLut* lut)
{
    Mem_Free(lut->texels);
    memset(lut, 0, sizeof(*lut));
}

pim_inline float2 VEC_CALL SampleBrdf(BrdfLut lut, float NoV, float roughness)
{
    return UvBilinearClamp_f2(lut.texels, lut.size, f2_v(NoV, roughness));
}
