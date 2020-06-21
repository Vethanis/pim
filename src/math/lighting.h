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

pim_inline float VEC_CALL BrdfAlpha(float roughness)
{
    return f1_max(roughness * roughness, 0.001f);
}

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
// cosTheta may be HoL or HoV (they are equivalent)
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
    float m2 = alpha * alpha;
    float f = 1.0f + NoH * (NoH * m2 - NoH);
    return m2 / f1_max(f * f * kPi, 0.001f);
}

// Specular 'G' term
// Correlated Smith
// represents the self shadowing and masking of microfacets in rough materials
// [Lagarde15, Page 12, Listing 2]
pim_inline float VEC_CALL G_SmithGGX(float NoL, float NoV, float alpha)
{
    float a2 = alpha * alpha;
    float v = NoL * sqrtf(a2 + (NoV - NoV * a2) * NoV);
    float l = NoV * sqrtf(a2 + (NoL - NoL * a2) * NoL);
    return 0.5f / f1_max(0.001f, v + l);
}

pim_inline float4 VEC_CALL SpecularBRDF(
    float4 F,
    float NoL,
    float NoV,
    float NoH,
    float alpha)
{
    float G = G_SmithGGX(NoL, NoV, alpha);
    float D = D_GTR(NoH, alpha);
    float4 DFG = f4_mulvs(F, D * G);
    float denom = f1_max(0.001f, 4.0f * NoL * NoV);
    return f4_divvs(DFG, denom);
}

pim_inline float VEC_CALL Fd_Lambert()
{
    return 1.0f / kPi;
}

// Diffuse term
// normalized Burley diffuse brdf
// [Lagarde15, Page 10, Listing 1]
pim_inline float VEC_CALL Fd_Burley(
    float NoL,
    float NoV,
    float HoV,
    float alpha)
{
    float energyBias = 0.5f * alpha;
    float energyFactor = f1_lerp(1.0f, 1.0f / 1.51f, alpha);
    float fd90 = energyBias + 2.0f * HoV * HoV * alpha;
    float lightScatter = F_Schlick1(1.0f, fd90, NoL);
    float viewScatter = F_Schlick1(1.0f, fd90, NoV);
    return (lightScatter * viewScatter * energyFactor) / kPi;
}

pim_inline float4 VEC_CALL DiffuseBRDF(
    float4 albedo,
    float4 F,
    float alpha,
    float metallic,
    float NoL,
    float NoV,
    float HoV)
{
    // metals are specular only
    float s = 1.0f - metallic;
    s *= Fd_Burley(NoL, NoV, HoV, alpha);
    // multiply by inverse of specular color, to remain energy conserving
    float4 c = f4_mul(albedo, f4_inv(F));
    return f4_mulvs(c, s);
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
    float4 H = f4_normalize3(f4_add(V, L));
    float NoV = f4_dotsat(N, V);
    float NoH = f4_dotsat(N, H);
    float NoL = f4_dotsat(N, L);
    float HoV = f4_dotsat(H, V);

    float alpha = BrdfAlpha(roughness);
    // LoH or VoH work as difference angle
    float4 F = F_Schlick(F_0(albedo, metallic), f4_1, HoV);
    float4 Fr = SpecularBRDF(F, NoL, NoV, NoH, alpha);
    float4 Fd = DiffuseBRDF(albedo, F, alpha, metallic, NoL, NoV, HoV);

    return f4_add(Fr, Fd);
}

// polynomial approximation for convolved specular DFG
// [Karis14]
pim_inline float4 VEC_CALL EnvBRDF(
    float4 F,
    float NoV,
    float alpha)
{
    const float4 c0 = { -1.0f, -0.0275f, -0.572f, 0.022f };
    const float4 c1 = { 1.0f, 0.0425f, 1.04f, -0.04f };
    float4 r = f4_add(f4_mulvs(c0, alpha), c1);
    float a004 = f1_min(r.x * r.x, exp2f(-9.28f * NoV)) * r.x + r.y;
    float a = -1.04f * a004 + r.z;
    float b = 1.04f * a004 + r.w;
    return f4_addvs(f4_mulvs(F, a), b);
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
    float alpha = BrdfAlpha(roughness);
    float NoV = f4_dotsat(N, V);

    float4 F = F_Schlick(F_0(albedo, metallic), f4_1, NoV);
    float4 Fr = EnvBRDF(F, NoV, alpha);
    Fr = f4_mul(Fr, specularGI);

    float4 Fd = f4_mul(albedo, f4_inv(F));
    Fd = f4_mulvs(Fd, 1.0f - metallic);
    Fd = f4_mul(Fd, diffuseGI);

    return f4_mulvs(f4_add(Fd, Fr), ao);
}

pim_inline float VEC_CALL SmoothDistanceAtt(
    float distSq, // distance squared between surface and light
    float attRadius) // attenuation radius
{
    float f1 = distSq / f1_max(0.001f, attRadius * attRadius);
    float f2 = f1_saturate(1.0f - f1 * f1);
    return f2 * f2;
}

pim_inline float VEC_CALL DistanceAtt(
    float4 L0, // unnormalized light vector
    float attRadius) // attenuation radius
{
    float distSq = f4_lengthsq3(L0);
    float att = 1.0f / f1_max(0.001f, distSq);
    att *= SmoothDistanceAtt(distSq, attRadius);
    return att;
}

pim_inline float VEC_CALL AngleAtt(
    float4 L, // normalized light vector
    float4 Ldir, // spotlight direction
    float cosInner, // cosine of inner angle
    float cosOuter, // cosine of outer angle
    float angleScale) // width of inner to outer transition
{
    float scale = 1.0f / f1_max(0.001f, cosInner - cosOuter);
    float offset = -cosOuter * angleScale;
    float cd = f4_dot3(Ldir, L);
    float att = f1_saturate(cd * scale + offset);
    return att * att;
}

pim_inline float4 VEC_CALL EvalPointLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 Lp, // light position
    float4 Lc, // light color
    float attRadius)
{
    float4 L0 = f4_sub(Lp, P);
    float4 L = f4_normalize3(L0);
    float4 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
    float att = f4_dotsat(N, L) * DistanceAtt(L0, attRadius);
    return f4_mul(brdf, f4_mulvs(Lc, att));
}

pim_inline float4 VEC_CALL EvalSunLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 sunDir) // sun direction
{
    // treat sun as a disk light
    // Earth's sun has an angular diameter between 0.526 and 0.545 degrees.
    const float kSunAngRadius = (0.536f / 2.0f) * kRadiansPerDegree;
    // Earth's sun has an illumiance between 105k and 114k lux.
    const float kSunLux = 110000.0f;
    // disk radius
    const float r = 0.00467746533f; // sinf(kSunAngRadius);
    // distance to disk
    const float d = 0.99998906059f; // cosf(kSunAngRadius);

    // D: diffuse lighting direction
    float4 D = sunDir;
    float4 R = f4_reflect3(f4_neg(V), N);
    float DoR = f4_dot3(D, R);

    // L: specular lighting direction
    float4 L = R;
    if (DoR < d)
    {
        float4 S = f4_sub(R, f4_mulvs(D, DoR));
        S = f4_normalize3(S);
        S = f4_mulvs(S, r);
        L = f4_add(f4_mulvs(D, d), S);
        L = f4_normalize3(L);
    }

    float illuminance = kSunLux * f4_dotsat(N, D);
    float4 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);

    return f4_mulvs(brdf, illuminance);
}

pim_inline float VEC_CALL SphereArea(float radius)
{
    return 4.0f * kPi * (radius * radius);
}

pim_inline float VEC_CALL DiskArea(float radius)
{
    return kPi * (radius * radius);
}

pim_inline float VEC_CALL TubeArea(float radius, float width)
{
    return (2.0f * kPi * radius * width) + (4.0f * kPi * radius * radius);
}

pim_inline float VEC_CALL RectArea(float width, float height)
{
    return width * height;
}

pim_inline float VEC_CALL SphereLumensToNits(float lumens, float radius)
{
    return lumens / (SphereArea(radius) * kPi);
}

pim_inline float VEC_CALL DiskLumensToNits(float lumens, float radius)
{
    return lumens / (DiskArea(radius) * kPi);
}

pim_inline float VEC_CALL TubeLumensToNits(
    float lumens,
    float radius,
    float width)
{
    return lumens / (TubeArea(radius, width) * kPi);
}

pim_inline float VEC_CALL RectLumensToNits(
    float lumens,
    float width,
    float height)
{
    return lumens / (RectArea(width, height) * kPi);
}

// [Lagarde15, Page 44, Listing 7]
pim_inline float VEC_CALL SphereIlluminance(
    float4 N, // unit surface normal
    float4 L,
    float distance,
    float radius)
{
    // do not saturate, needed for horizon handling.
    float c = f4_dot3(N, L);

    float h = distance / radius;
    h = f1_max(h, 1.0f);
    float h2 = h * h;

    if ((h * c) >= 1.0f)
    {
        return kPi * (c / h2);
    }
    else
    {
        // https://www.desmos.com/calculator/t10vnyq13k
        // cos(t) = dot(a, b)
        // 1. sin(t) = length(cross(a, b))
        // 2. sin(t) = sqrt(1 - cos(t)^2)

        float s = sqrtf(1.0f - c * c);
        s = f1_max(s, 0.001f);

        float u = sqrtf(h2 - 1.0f);
        u = f1_max(u, 0.001f);

        float v = -u * (c / s);
        float w = s * sqrtf(1.0f - v * v);

        float t1 = 1.0f / h2;
        float t2 = c * acosf(v) - u * w;
        float t3 = atanf(w / u);

        return t1 * t2 + t3;
    }
}

pim_inline float4 VEC_CALL EvalSphereLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 lightPos, // light position
    float4 lightColor, // light color
    float lightRadius)
{
    float4 L0 = f4_sub(lightPos, P);
    float distance = f4_distance3(lightPos, P);
    float4 L = f4_divvs(L0, distance);

    lightRadius = f1_max(0.01f, lightRadius);
    distance = f1_max(distance, lightRadius);

    float4 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
    float att = SphereIlluminance(N, L, distance, lightRadius);

    return f4_mul(brdf, f4_mulvs(lightColor, att));
}

PIM_C_END
