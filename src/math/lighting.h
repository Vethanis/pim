#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float4_funcs.h"
#include "rendering/texture.h"

typedef struct BrdfLut_s
{
    int2 size;
    float2* pim_noalias texels;
} BrdfLut;

BrdfLut BakeBRDF(int2 size, u32 numSamples);
void FreeBrdfLut(BrdfLut* lut);

// alpha constant
// Disney's reparameterization of alpha = roughness^2
pim_inline float VEC_CALL GGX_a(float roughness)
{
    return roughness * roughness;
}

// k constant under direct lighting
pim_inline float VEC_CALL GGX_kDirect(float roughness)
{
    float k = roughness + 1.0f;
    k = k * k;
    return k * 0.125f;
}

// k constant under image based lighting
pim_inline float VEC_CALL GGX_kIBL(float roughness)
{
    float a = GGX_a(roughness);
    float k = (a * a) * 0.5f;
    return k;
}

// geometry term
// self shadowing effect from rough surfaces
// Schlick approximation fit to Smith's GGX model
pim_inline float VEC_CALL GGX_G(float NoV, float NoL, float k)
{
    float invK = 1.0f - k;
    float t1 = NoV / (NoV * invK + k);
    float t2 = NoL / (NoL * invK + k);
    return t1 * t2;
}

// normal distribution function
// amount of surface's microfacets are facing the H vector
// Trowbridge-Reitz GGX
pim_inline float VEC_CALL GGX_D(float NoH, float a)
{
    float a2 = a * a;
    float NoH2 = NoH * NoH;
    float d = NoH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (kPi * d * d);
}

// base reflectivity for a surface viewed head-on
pim_inline float4 VEC_CALL GGX_F0(float4 albedo, float metallic)
{
    return f4_lerp(f4_s(0.04f), albedo, metallic);
}

// direct fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float4 VEC_CALL GGX_F(float cosTheta, float4 F0)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerp(F0, f4_s(1.0f), t5);
}

// indirect fresnel term
// ratio of light reflection vs refraction, at a given angle
// Fresnel-Schlick approximation
pim_inline float4 VEC_CALL GGX_FI(float cosTheta, float4 F0, float roughness)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerp(F0, f4_max(f4_s(1.0f - roughness), F0), t5);
}

// Cook-Torrance BRDF
float4 VEC_CALL DirectBRDF(
    float4 V,           // unit view vector, points from position to eye
    float4 L,           // unit light vector, points from position to light
    float4 radiance,    // light color * intensity * attenuation
    float4 N,           // unit normal vector of surface
    float4 albedo,      // surface color (color! not diffuse map!)
    float roughness,    // perceptual roughness
    float metallic);     // perceptual metalness

float4 VEC_CALL IndirectBRDF(
    BrdfLut lut,
    float4 V,           // unit view vector pointing from surface to eye
    float4 N,           // unit normal vector pointing outward from surface
    float4 diffuseGI,   // low frequency scene irradiance
    float4 specularGI,  // high frequency scene irradiance
    float4 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao);          // 1 - ambient occlusion (affects gi only)

PIM_C_END
