#include "math/lighting.h"
#include "math/sampling.h"
#include "rendering/sampler.h"
#include "allocator/allocator.h"

// Cook-Torrance BRDF
pim_optimize
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

    const float a = GGX_a(roughness);
    const float k = GGX_kDirect(roughness);

    const float D = GGX_D(NoH, a);
    const float G = GGX_G(NoV, NoL, k);
    const float4 F = GGX_F(NoV, GGX_F0(albedo, metallic));

    const float4 DGF = f4_mulvs(F, D * G);
    const float4 specular = f4_divvs(DGF, f1_max(0.001f, 4.0f * NoV * NoL));
    const float4 kD = f4_mulvs(f4_sub(f4_1, F), 1.0f - metallic);
    const float4 diffuse = f4_divvs(f4_mul(kD, albedo), kPi);
    return f4_mul(f4_add(diffuse, specular), f4_mulvs(radiance, NoL));
}

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// ideally this should be baked into a LUT
pim_optimize
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

    const float a = GGX_a(roughness);
    const float k = GGX_kIBL(roughness);

    float2 result = { 0.0f, 0.0f };

    const float weight = 1.0f / numSamples;
    for (u32 i = 0; i < numSamples; ++i)
    {
        const float2 Xi = Hammersley2D(i, numSamples);
        const float4 H = ImportanceSampleGGX(Xi, a);
        const float VoH = f1_max(0.0f, f4_dot3(V, H));
        const float NoH = f1_max(0.0f, H.z);
        const float4 L = f4_reflect(f4_neg(V), H);
        const float NoL = L.z;

        if (NoL > 0.0f)
        {
            const float G = GGX_G(NoV, NoL, k);
            const float GVis = (G * VoH) / (NoH * NoV);
            float Fc = 1.0f - VoH;
            Fc = Fc * Fc * Fc * Fc * Fc;

            result.x += weight * ((1.0f - Fc) * GVis);
            result.y += weight * (Fc * GVis);
        }
    }

    return result;
}

pim_optimize
BrdfLut BakeBRDF(int2 size, u32 numSamples)
{
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);
    ASSERT(numSamples > 64);

    float2* pim_noalias texels = perm_malloc(sizeof(texels[0]) * size.x * size.y);

    const float dx = 1.0f / size.x;
    const float dy = 1.0f / size.y;
    for (i32 y = 0; y < size.x; ++y)
    {
        for (i32 x = 0; x < size.y; ++x)
        {
            float u = (x + 0.5f) * dx;
            float v = (y + 0.5f) * dy;
            i32 i = y * size.x + x;
            texels[i] = IntegrateBRDF(u, v, numSamples);
        }
    }

    BrdfLut lut;
    lut.texels = texels;
    lut.size = size;
    return lut;
}

void FreeBrdfLut(BrdfLut* lut)
{
    pim_free(lut->texels);
    lut->texels = NULL;
    lut->size = i2_s(0);
}

pim_optimize
pim_inline float2 Lut_Nearesti2(BrdfLut lut, int2 coord)
{
    coord = Tex_ClampCoord(lut.size, coord);
    i32 i = Tex_CoordToIndex(lut.size, coord);
    return lut.texels[i];
}

pim_optimize
pim_inline float2 VEC_CALL SampleBrdf(BrdfLut lut, float roughness, float NoV)
{
    float2 uv = f2_v(roughness, NoV);
    float2 coordf = Tex_UvToCoordf(lut.size, uv);
    float2 frac = f2_frac(coordf);
    int2 ia = f2_i2(coordf);
    int2 ib = { ia.x + 1, ia.y + 0 };
    int2 ic = { ia.x + 0, ia.y + 1 };
    int2 id = { ia.x + 1, ia.y + 1 };
    float2 a = Lut_Nearesti2(lut, ia);
    float2 b = Lut_Nearesti2(lut, ib);
    float2 c = Lut_Nearesti2(lut, ic);
    float2 d = Lut_Nearesti2(lut, id);
    float2 e = f2_lerp(f2_lerp(a, b, frac.x), f2_lerp(c, d, frac.x), frac.y);
    return e;
}

pim_optimize
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
