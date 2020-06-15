#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/cubemap.h"

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
static float2 VEC_CALL IntegrateBRDF(
    float NoV,
    float roughness,
    u32 numSamples)
{
    float alpha = roughness * roughness;

    const float4 V =
    {
        sqrtf(1.0f - NoV * NoV),
        0.0f,
        NoV,
        0.0f,
    };
    const float4 I = f4_neg(V);

    const float4 N = { 0.0f, 0.0f, 1.0f, 0.0f };
    const float3x3 TBN = NormalToTBN(N);
    float2 result = { 0.0f, 0.0f };

    const float weight = 1.0f / numSamples;
    for (u32 i = 0; i < numSamples; ++i)
    {
        float2 Xi = Hammersley2D(i, numSamples);
        float4 H = TbnToWorld(TBN, SampleGGXMicrofacet(Xi, roughness));
        float4 L = f4_normalize3(f4_reflect3(I, H));

        float NoL = L.z;
        float NoH = f1_max(0.0f, H.z);
        float VoH = f1_max(0.0f, f4_dot3(V, H));

        if (NoL > 0.0f)
        {
            float G = G_SmithGGX(NoL, NoV, alpha);
            float GVis = (G * VoH) / f1_max(NoH * NoV, f16_eps);
            float Fc = 1.0f - VoH;
            Fc = Fc * Fc * Fc * Fc * Fc;

            result.x += weight * ((1.0f - Fc) * GVis);
            result.y += weight * (Fc * GVis);
        }
    }

    return result;
}

typedef struct bakebrdf_s
{
    task_t task;
    BrdfLut lut;
    u32 numSamples;
} bakebrdf_t;

static void BrdfBakeFn(task_t* pBase, i32 begin, i32 end)
{
    bakebrdf_t* task = (bakebrdf_t*)pBase;
    float2* pim_noalias texels = task->lut.texels;
    const int2 size = task->lut.size;
    const u32 numSamples = task->numSamples;

    for (i32 i = begin; i < end; ++i)
    {
        i32 x = i % size.x;
        i32 y = i / size.x;
        float2 uv = CoordToUv(size, i2_v(x, y));
        float NoV = uv.x;
        float roughness = uv.y;
        texels[i] = IntegrateBRDF(NoV, roughness, numSamples);
    }
}

BrdfLut BakeBRDF(int2 size, u32 numSamples)
{
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);
    ASSERT(numSamples > 64);

    BrdfLut lut;
    lut.texels = perm_malloc(sizeof(lut.texels[0]) * size.x * size.y);;
    lut.size = size;

    bakebrdf_t task = { 0 };
    task.lut = lut;
    task.numSamples = numSamples;
    task_submit((task_t*)&task, BrdfBakeFn, size.x * size.y);
    task_sys_schedule();
    task_await((task_t*)&task);

    return lut;
}

void FreeBrdfLut(BrdfLut* lut)
{
    pim_free(lut->texels);
    lut->texels = NULL;
    lut->size = i2_s(0);
}

pim_inline float2 VEC_CALL SampleBrdf(BrdfLut lut, float NoV, float roughness)
{
    return UvBilinearClamp_f2(lut.texels, lut.size, f2_v(NoV, roughness));
}
