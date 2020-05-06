#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct BrdfLut_s
{
    float2* ptr;
    i32 width;
    i32 height;
} BrdfLut;

BrdfLut BakeBRDF(i32 width, i32 height, u32 numSamples);
void FreeBRDF(BrdfLut* lut);

// Cook-Torrance BRDF
float3 VEC_CALL DirectBRDF(
    float3 V,           // unit view vector, points from position to eye
    float3 L,           // unit light vector, points from position to light
    float3 radiance,    // light color * intensity * attenuation
    float3 N,           // unit normal vector of surface
    float3 albedo,      // surface color (color! not diffuse map!)
    float roughness,    // perceptual roughness
    float metallic);     // perceptual metalness

float3 VEC_CALL IndirectBRDF(
    BrdfLut lut,
    float3 V,           // unit view vector pointing from surface to eye
    float3 N,           // unit normal vector pointing outward from surface
    float3 diffuseGI,   // low frequency scene irradiance
    float3 specularGI,  // high frequency scene irradiance
    float3 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao);          // 1 - ambient occlusion (affects gi only)

PIM_C_END
