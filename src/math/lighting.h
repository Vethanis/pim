#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"
#include "rendering/texture.h"
#include "rendering/cubemap.h"

PIM_C_BEGIN

typedef struct BrdfLut_s
{
    int2 size;
    float2* pim_noalias texels;
} BrdfLut;

BrdfLut BakeBRDF(int2 size, u32 numSamples);
void FreeBrdfLut(BrdfLut* lut);

void Prefilter(const Cubemap* src, Cubemap* dst, u32 sampleCount);

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
