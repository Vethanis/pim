#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float3_funcs.h"

// normal distribution function
// amount of surface's microfacets are facing the H vector
// Trowbridge-Reitz GGX approximation
pim_inline float VEC_CALL GGX_D(float NoH, float roughness)
{
    float r2 = roughness * roughness;
    float r4 = r2 * r2;
    float NoH2 = NoH * NoH;
    float t = NoH2 * (r4 - 1.0f) + 1.0f;
    return r4 / (kPi * t * t);
}

// base reflectivity for a surface viewed head-on
pim_inline float3 VEC_CALL GGX_F0(float3 albedo, float metallic)
{
    const float3 F0 = { 0.04f, 0.04f, 0.04f };
    return f3_lerp(F0, albedo, metallic);
}

// direct fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float3 VEC_CALL GGX_F(float cosTheta, float3 F0)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f3_lerp(F0, f3_1, t5);
}

// indirect fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float3 VEC_CALL GGX_FI(float cosTheta, float3 F0, float roughness)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f3_lerp(F0, f3_max(F0, f3_s(1.0f - roughness)), t5);
}

// direct geometry term
// self shadowing effect from rough surfaces
// Smith's Schlick-GGX approximation
pim_inline float VEC_CALL GGX_G(float NoV, float NoL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) * 0.125f;
    float revK = 1.0f - k;
    float a = NoV / (k + NoV * revK);
    float b = NoL / (k + NoL * revK);
    return a * b;
}

// indirect geometry term
// self shadowing effect from rough surfaces
// Smith's Schlick-GGX approximation
pim_inline float VEC_CALL GGX_GI(float NoV, float NoL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) * 0.5f;
    float revK = 1.0f - k;
    float a = NoV / (k + NoV * revK);
    float b = NoL / (k + NoL * revK);
    return a * b;
}

// Cook-Torrance BRDF
pim_inline float3 VEC_CALL DirectBRDF(
    float3 V,           // unit view vector, points from position to eye
    float3 L,           // unit light vector, points from position to light
    float3 radiance,    // light color * intensity * attenuation
    float3 N,           // unit normal vector of surface
    float3 albedo,      // surface color (color! not diffuse map!)
    float roughness,    // perceptual roughness
    float metallic)     // perceptual metalness
{
    const float3 H = f3_normalize(f3_add(V, L));
    const float NoH = f1_max(0.0f, f3_dot(N, H));
    const float HoV = f1_max(0.0f, f3_dot(H, V));
    const float NoV = f1_max(0.0f, f3_dot(N, V));
    const float NoL = f1_max(0.0f, f3_dot(N, L));
    const float D = GGX_D(NoH, roughness);
    const float G = GGX_G(NoV, NoL, roughness);
    const float3 F = GGX_F(HoV, GGX_F0(albedo, metallic));
    const float3 DGF = f3_mulvs(F, D * G);
    const float3 specular = f3_divvs(DGF, f1_max(0.0001f, 4.0f * NoV * NoL));
    const float3 kD = f3_mulvs(f3_sub(f3_1, F), 1.0f - metallic);
    const float3 diffuse = f3_divvs(f3_mul(kD, albedo), kPi);
    return f3_mul(f3_add(diffuse, specular), f3_mulvs(radiance, NoL));
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
pim_inline float VEC_CALL RadicalInverse_VdC(u32 bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return (float)(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

// generates a stratified random 2D variable with range [0, 1]
pim_inline float2 VEC_CALL Hammersley2D(u32 i, u32 N)
{
    float2 c = { (float)i / (float)N, RadicalInverse_VdC(i) };
    return c;
}

// outputs a tangent space half vector of the NDF for given roughness
pim_inline float3 ImportanceSampleGGX(float2 Xi, float roughness)
{
    float a = roughness * roughness;
    float phi = kTau * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const float3 H =
    {
        cosf(phi) * sinTheta,
        sinf(phi) * sinTheta,
        cosTheta,
    };
    // TODO: might not need normalized
    return f3_normalize(H);
}

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// ideally this should be baked into a LUT
pim_inline float2 VEC_CALL IntegrateBDF(float roughness, float NoV, u32 numSamples)
{
    const float3 V =
    {
        sqrtf(1.0f - NoV * NoV),
        0.0f,
        NoV,
    };

    float2 result = { 0.0f, 0.0f };

    const float weight = 1.0f / numSamples;
    for (u32 i = 0; i < numSamples; ++i)
    {
        const float2 Xi = Hammersley2D(i, numSamples);
        const float3 H = ImportanceSampleGGX(Xi, roughness);
        const float VoH = f1_max(0.0f, f3_dot(V, H));
        const float NoH = f1_max(0.0f, H.z);
        const float3 L = f3_reflect(f3_neg(V), H);
        const float NoL = L.z;

        if (NoL > 0.0f)
        {
            const float G = GGX_GI(NoV, NoL, roughness);
            const float GVis = (G * VoH) / (NoH * NoV);
            float Fc = 1.0f - VoH;
            Fc = Fc * Fc * Fc * Fc * Fc;

            result.x += weight * ((1.0f - Fc) * GVis);
            result.y += weight * (Fc * GVis);
        }
    }

    return result;
}

pim_inline float3 VEC_CALL IndirectBRDF(
    float3 V,           // unit view vector pointing from surface to eye
    float3 N,           // unit normal vector pointing outward from surface
    float3 diffuseGI,   // low frequency scene irradiance
    float3 specularGI,  // high frequency scene irradiance
    float3 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao)           // 1 - ambient occlusion (affects gi only)
{
    const float NoV = f1_max(0.0f, f3_dot(N, V));
    const float3 F = GGX_FI(NoV, GGX_F0(albedo, metallic), roughness);
    const float3 kD = f3_mulvs(f3_sub(f3_1, F), 1.0f - metallic);
    const float3 diffuse = f3_mul(albedo, diffuseGI);
    const float2 brdf = IntegrateBDF(roughness, NoV, 16);
    const float3 specular = f3_mul(specularGI, f3_addvs(f3_mulvs(F, brdf.x), brdf.y));
    return f3_mulvs(f3_add(f3_mul(kD, diffuse), specular), ao);
}

PIM_C_END
