#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/cubemap.h"

// Cook-Torrance BRDF
float4 VEC_CALL DirectBRDF(
    float4 V,           // unit view vector, points from position to eye
    float4 L,           // unit light vector, points from position to light
    float4 radiance,    // light color * intensity * attenuation
    float4 N,           // unit normal vector of surface
    float4 albedo,      // surface color (color! not diffuse map!)
    float roughness,    // perceptual roughness
    float metallic)     // perceptual metalness
{
    const float4 H = f4_normalize3(f4_add(V, L));
    const float NoH = f1_max(0.0f, f4_dot3(N, H));
    const float NoV = f1_max(0.0f, f4_dot3(N, V));
    const float NoL = f1_max(0.0f, f4_dot3(N, L));

    const float D = GGX_D(NoH, roughness);
    const float G = GGX_GD(NoV, NoL, roughness);
    const float4 F = GGX_F(NoV, GGX_F0(albedo, metallic));
    const float4 DGF = f4_mulvs(F, D * G);

    const float4 specular = f4_divvs(DGF, f1_max(0.001f, 4.0f * NoV * NoL));
    const float4 kD = f4_mulvs(f4_sub(f4_1, F), 1.0f - metallic);
    const float4 diffuse = f4_divvs(f4_mul(kD, albedo), kPi);

    return f4_mul(f4_add(diffuse, specular), f4_mulvs(radiance, NoL));
}

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
static float2 VEC_CALL IntegrateBRDF(
    float NoV,
    float roughness,
    u32 numSamples)
{
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
            float G = GGX_GI(NoV, NoL, roughness);
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

// https://knarkowicz.wordpress.com/2014/12/27/analytical-dfg-term-for-ibl/
static float4 VEC_CALL EnvDFGPolynomial(float4 specular, float NoV, float roughness)
{
    float x = 1.0f - roughness;
    float y = NoV;

    const float b1 = -0.1688f;
    const float b2 = 1.895f;
    const float b3 = 0.9903f;
    const float b4 = -4.853f;
    const float b5 = 8.404f;
    const float b6 = -5.069f;
    float bias = f1_saturate(f1_min(b1 * x + b2 * x * x, b3 + b4 * y + b5 * y * y + b6 * y * y * y));

    float d0 = 0.6045f;
    float d1 = 1.699f;
    float d2 = -0.5228f;
    float d3 = -3.603f;
    float d4 = 1.404f;
    float d5 = 0.1939f;
    float d6 = 2.661f;
    float delta = f1_saturate(d0 + d1 * x + d2 * y + d3 * x * x + d4 * x * y + d5 * y * y + d6 * x * x * x);
    float scale = delta - bias;

    bias *= f1_saturate(50.0f * specular.y);
    return f4_addvs(f4_mulvs(specular, scale), bias);
}

float4 VEC_CALL IndirectBRDF(
    float4 V,           // unit view vector pointing from surface to eye
    float4 N,           // unit normal vector pointing outward from surface
    float4 diffuseGI,   // low frequency scene irradiance
    float4 specularGI,  // high frequency scene irradiance
    float4 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao)           // 1 - ambient occlusion (affects gi only)
{
    float NoV = f1_max(0.0f, f4_dot3(N, V));

    float4 F = GGX_FI(NoV, GGX_F0(albedo, metallic), roughness);
    F = f4_mulvs(F, 0.075f); // fresnel too stronk

    float4 kD = f4_mulvs(f4_inv(F), 1.0f - metallic);
    float4 diffuse = f4_mul(albedo, diffuseGI);
    float4 specular = EnvDFGPolynomial(f4_mul(specularGI, F), NoV, roughness);

    return f4_mulvs(f4_add(f4_mul(kD, diffuse), specular), ao);
}
