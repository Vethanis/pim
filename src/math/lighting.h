#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"

PIM_C_BEGIN

// References:
// ----------------------------------------------------------------------------
// [Burley12]: https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [Karis13]: https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [Karis14]: https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
// [Lagarde15]: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

// Legend:
// ----------------------------------------------------------------------------
// FRESNEL:
//   f0: fresnel reflectance at normal incidence
//   f90: fresnel reflectance at tangent incidence
// ROUGHNESS:
//   perceptually linear roughness: material.roughness
//   alpha, or a: pow(material.roughness, 2)
//   alpha_linear or a_lin: material.roughness
//   """
//     [Burley12]
//     For  roughness, we found that mapping alpha=roughness^2 results
//     in a more perceptually linear change in the roughness.
//     Without this remapping, very small and non-intuitive values were required
//     for matching shiny materials. Also, interpolating between a rough and
//     smooth material would always produce a rough result.
//   """

typedef struct BrdfLut_s
{
    int2 size;
    float2* pim_noalias texels;
} BrdfLut;

BrdfLut BakeBRDF(int2 size, u32 numSamples);
void FreeBrdfLut(BrdfLut* lut);

// Specular reflectance at normal incidence (N==L)
// """
//   [Burley12, Page 16, Section 5.5]
//   The constant, F0, represents the specular reflectance at normal incidence
//   and is achromatic for dielectrics and chromatic (tinted) for metals.
//   The actual value depends on the index of refraction.
// """
pim_inline float4 VEC_CALL F_0(float4 albedo, float metallic)
{
    return f4_lerpvs(f4_s(0.04f), albedo, metallic);
}

// Specular 'F' term
// represents the ratio of reflected to refracted light
// """
//   [Burley12, Page 16, Section 5.5]
//   The  Fresnel  function  can  be  seen  as  interpolating  (non-linearly)
//   between  the  incident  specular reflectance and unity at grazing angles.
//   Note that the response becomes achromatic at grazing incidenceas all light
//   is reflected.
// """
pim_inline float4 VEC_CALL F_Schlick(float4 f0, float4 f90, float cosTheta)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerpvs(f0, f90, t5);
}

pim_inline float VEC_CALL F_Schlick1(float f0, float f90, float cosTheta)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f1_lerp(f0, f90, t5);
}

// Specular 'D' term
// represents the normal distribution function
// GGX Trowbridge-Reitz approximation
// [Karis13, Page 3, Figure 3]
pim_inline float VEC_CALL D_GTR(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float f = f1_lerp(1.0f, a2, NoH * NoH);
    return a2 / (f * f * kPi);
}

// Specular 'G' term
// Correlated Smith
// represents the self shadowing and masking of microfacets in rough materials
// [Lagarde15, Page 12, Listing 2]
pim_inline float VEC_CALL G_SmithGGX(float NoL, float NoV, float alpha)
{
    float a2 = alpha * alpha;
    float NoL2 = NoL * NoL;
    float NoV2 = NoV * NoV;
    float v = sqrtf(1.0f + a2 * (1.0f - NoL2) / NoL2) - 1.0f;
    float l = sqrtf(1.0f + a2 * (1.0f - NoV2) / NoV2) - 1.0f;
    float g = 1.0f / (1.0f + 0.5f * v + 0.5f * l);
    return g;
}

pim_inline float VEC_CALL Fd_Lambert() { return 1.0f / kPi; }

// Diffuse term
// normalized Burley diffuse brdf
// [Lagarde15, Page 10, Listing 1]
pim_inline float VEC_CALL Fd_Burley(
    float NoL,
    float NoV,
    float LoH,
    float roughness) // perceptually linear roughness
{
    float energyBias = f1_lerp(0.0f, 0.5f, roughness);
    float energyFactor = f1_lerp(1.0f, 1.0f / 1.51f, roughness);
    float fd90 = energyBias + 2.0f * LoH * LoH * roughness;
    float lightScatter = F_Schlick1(1.0f, fd90, NoL);
    float viewScatter = F_Schlick1(1.0f, fd90, NoV);
    return (lightScatter * viewScatter * energyFactor) / kPi;
}

// multiply output by radiance of light
pim_inline float4 VEC_CALL DirectBRDF(
    float4 V,
    float4 L,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic)
{
    float alpha = roughness * roughness;

    float4 H = f4_normalize3(f4_add(V, L));
    float NoV = f1_saturate(f4_dot3(N, V));
    float NoH = f1_saturate(f4_dot3(N, H));
    float NoL = f1_saturate(f4_dot3(N, L));
    float LoH = f1_saturate(f4_dot3(L, H));

    float4 f0 = F_0(albedo, metallic);
    float4 F = F_Schlick(f0, f4_1, LoH); // LoH or VoH work as difference angle
    float G = G_SmithGGX(NoL, NoV, alpha);
    float D = D_GTR(NoH, alpha);
    float4 Fr = f4_divvs(f4_mulvs(F, D * G), f1_max(kEpsilon, 4.0f * NoL * NoV));

    float4 Fd = f4_mul(albedo, f4_mulvs(f4_inv(F), (1.0f - metallic) / kPi));

    return f4_mulvs(f4_add(Fr, Fd), NoL);
}

// polynomial approximation for convolved specular DFG
// [Karis14]
pim_inline float4 VEC_CALL EnvDFG(
    float4 f0,
    float cosTheta,
    float alpha)
{
    const float4 c0 = { -1.0f, -0.0275f, -0.572f, 0.022f };
    const float4 c1 = { 1.0f, 0.0425f, 1.04f, -0.04f };
    float4 r = f4_add(f4_mulvs(c0, alpha), c1);
    float a004 = f1_min(r.x * r.x, exp2f(-9.28f * cosTheta)) * r.x + r.y;
    float2 AB = f2_add(f2_mulvs(f2_v(-1.04f, 1.04f), a004), f2_v(r.z, r.w));
    return f4_addvs(f4_mulvs(f0, AB.x), AB.y);
}

pim_inline float4 VEC_CALL IndirectBRDF(
    float4 V,           // unit view vector pointing from surface to eye
    float4 N,           // unit normal vector pointing outward from surface
    float4 diffuseGI,   // low frequency scene irradiance
    float4 specularGI,  // high frequency scene irradiance
    float4 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao)           // 1 - ambient occlusion (affects gi only)
{
    float alpha = roughness * roughness;
    float cosTheta = f1_saturate(f4_dot3(N, V));

    float4 f0 = F_0(albedo, metallic);
    float4 F = F_Schlick(f0, f4_1, cosTheta);
    float4 Fr = EnvDFG(f0, cosTheta, alpha);
    Fr = f4_mul(Fr, specularGI);

    float4 Fd = f4_mul(albedo, f4_mulvs(f4_inv(F), 1.0f - metallic));
    Fd = f4_mul(Fd, diffuseGI);

    return f4_mulvs(f4_add(Fd, Fr), ao);
}

PIM_C_END
