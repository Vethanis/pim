#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"
#include "threading/task.h"
#include "rendering/cubemap.h"

// geometry term
// self shadowing effect from rough surfaces
// Schlick approximation fit to Smith's GGX model
pim_inline float VEC_CALL GGX_G(float NoV, float NoL, float roughness)
{
    float a = roughness * roughness;
    float k = a * 0.5f;
    float t1 = NoV / f1_lerp(k, 1.0f, NoV);
    float t2 = NoL / f1_lerp(k, 1.0f, NoL);
    return t1 * t2;
}

// normal distribution function
// amount of surface's microfacets are facing the H vector
// Trowbridge-Reitz GGX
pim_inline float VEC_CALL GGX_D(float NoH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = f1_lerp(1.0f, a2, NoH * NoH);
    return a2 / (kPi * d * d);
}

// base reflectivity for a surface viewed head-on
pim_inline float4 VEC_CALL GGX_F0(float4 albedo, float metallic)
{
    return f4_lerpvs(f4_s(0.04f), albedo, metallic);
}

// direct fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float4 VEC_CALL GGX_F(float cosTheta, float4 F0)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerpvs(F0, f4_s(1.0f), t5);
}

// indirect fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float4 VEC_CALL GGX_FI(float cosTheta, float4 F0, float roughness)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerpvs(F0, f4_max(f4_s(1.0f - roughness), F0), t5);
}

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
    const float G = GGX_G(NoV, NoL, roughness);
    const float4 F = GGX_F(NoV, GGX_F0(albedo, metallic));

    const float4 DGF = f4_mulvs(F, D * G);
    const float4 specular = f4_divvs(DGF, f1_max(0.001f, 4.0f * NoV * NoL));
    const float4 kD = f4_mulvs(f4_sub(f4_1, F), 1.0f - metallic);
    const float4 diffuse = f4_divvs(f4_mul(kD, albedo), kPi);
    return f4_mul(f4_add(diffuse, specular), f4_mulvs(radiance, NoL));
}

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// ideally this should be baked into a LUT
static float2 VEC_CALL IntegrateBRDF(
    float roughness,
    float NoV,
    u32 numSamples)
{
    const float4 V =
    {
        sqrtf(1.0f - NoV * NoV),
        0.0f,
        NoV,
        0.0f,
    };

    float2 result = { 0.0f, 0.0f };

    const float weight = 1.0f / numSamples;
    for (u32 i = 0; i < numSamples; ++i)
    {
        const float2 Xi = Hammersley2D(i, numSamples);
        const float4 H = ImportanceSampleGGX(Xi, roughness);
        const float VoH = f1_max(0.0f, f4_dot3(V, H));
        const float NoH = f1_max(0.0f, H.z);
        const float4 L = f4_normalize3(f4_reflect3(f4_neg(V), H));
        const float NoL = L.z;

        if (NoL > 0.0f)
        {
            const float G = GGX_G(NoV, NoL, roughness);
            const float GVis = (G * VoH) / (NoH * NoV);
            float Fc = 1.0f - VoH;
            Fc = Fc * Fc * Fc * Fc * Fc;

            result.x += weight * ((1.0f - Fc) * GVis);
            result.y += weight * (Fc * GVis);
        }
    }

    return result;
}

static float4 VEC_CALL PrefilterEnvMap(
    const Cubemap* cm,
    float3x3 TBN,
    float4 N,
    u32 sampleCount,
    float roughness)
{
    float weight = 0.0f;
    float4 light = f4_0;
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float2 Xi = Hammersley2D(i, sampleCount);
        float4 H = TbnToWorld(TBN, ImportanceSampleGGX(Xi, roughness));
        const float4 L = f4_normalize3(f4_reflect3(f4_neg(N), H));
        const float NoL = f4_dot3(L, N);
        if (NoL > 0.0f)
        {
            float4 sample = Cubemap_Read(cm, L, 0.0f);
            sample = f4_mulvs(sample, NoL);
            light = f4_add(light, sample);
            weight += NoL;
        }
    }
    if (weight > 0.0f)
    {
        light = f4_divvs(light, weight);
    }
    return light;
}

typedef struct bakebrdf_s
{
    task_t task;
    BrdfLut lut;
    u32 numSamples;
} bakebrdf_t;

typedef struct prefilter_s
{
    task_t task;
    const Cubemap* src;
    Cubemap* dst;
    i32 mip;
    i32 size;
    u32 sampleCount;
} prefilter_t;

static void PrefilterFn(task_t* pBase, i32 begin, i32 end)
{
    prefilter_t* task = (prefilter_t*)pBase;
    const Cubemap* src = task->src;
    Cubemap* dst = task->dst;

    const u32 sampleCount = task->sampleCount;
    const i32 mipCount = i1_min(src->mipCount, dst->mipCount);
    const i32 size = task->size;
    const i32 mip = task->mip;
    const i32 len = size * size;
    const float roughness = (float)mip / (float)(mipCount - 1);

    for (i32 i = begin; i < end; ++i)
    {
        i32 face = i / len;
        i32 fi = i % len;
        int2 coord = { fi % size, fi / size };

        float4 N = Cubemap_CalcDir(size, face, coord, f2_0);
        float3x3 TBN = NormalToTBN(N);
        float4 light = PrefilterEnvMap(src, TBN, N, sampleCount, roughness);
        Cubemap_WriteMip(dst, face, coord, mip, light);
    }
}

void Prefilter(const Cubemap* src, Cubemap* dst, u32 sampleCount)
{
    ASSERT(src);
    ASSERT(dst);

    const i32 mipCount = i1_min(5, i1_min(src->mipCount, dst->mipCount));
    const i32 size = i1_min(src->size, dst->size);

    prefilter_t* last = NULL;
    for (i32 m = 0; m < mipCount; ++m)
    {
        i32 mSize = size >> m;
        i32 len = mSize * mSize * 6;

        prefilter_t* task = tmp_calloc(sizeof(*task));
        task->src = src;
        task->dst = dst;
        task->mip = m;
        task->size = mSize;
        task->sampleCount = sampleCount;
        task_submit((task_t*)task, PrefilterFn, len);
        last = task;
    }

    if (last)
    {
        task_sys_schedule();
        task_await((task_t*)last);
    }
}

static void BrdfBakeFn(task_t* pBase, i32 begin, i32 end)
{
    bakebrdf_t* task = (bakebrdf_t*)pBase;
    float2* pim_noalias texels = task->lut.texels;
    const int2 size = task->lut.size;
    const u32 numSamples = task->numSamples;

    const float dx = 1.0f / size.x;
    const float dy = 1.0f / size.y;
    for (i32 i = begin; i < end; ++i)
    {
        i32 x = i % size.x;
        i32 y = i / size.x;
        float u = (x + 0.5f) * dx;
        float v = (y + 0.5f) * dy;
        texels[i] = IntegrateBRDF(u, v, numSamples);
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

pim_inline float2 VEC_CALL SampleBrdf(BrdfLut lut, float roughness, float NoV)
{
    return UvBilinearClamp_f2(lut.texels, lut.size, f2_v(roughness, NoV));
}

float4 VEC_CALL IndirectBRDF(
    BrdfLut lut,
    float4 V,           // unit view vector pointing from surface to eye
    float4 N,           // unit normal vector pointing outward from surface
    float4 diffuseGI,   // low frequency scene irradiance
    float4 specularGI,  // high frequency scene irradiance
    float4 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao)           // 1 - ambient occlusion (affects gi only)
{
    const float NoV = f1_max(0.0f, f4_dot3(N, V));

    const float4 F = GGX_FI(NoV, GGX_F0(albedo, metallic), roughness);

    const float4 kD = f4_mulvs(f4_sub(f4_1, F), 1.0f - metallic);
    const float4 diffuse = f4_mul(albedo, diffuseGI);

    const float2 brdf = SampleBrdf(lut, roughness, NoV);
    const float4 specular = f4_mul(specularGI, f4_addvs(f4_mulvs(F, brdf.x), brdf.y));

    return f4_mulvs(f4_add(f4_mul(kD, diffuse), specular), ao);
}
