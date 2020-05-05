#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/scalar.h"
#include "math/float3_funcs.h"

// Computes a radical inverse with base 2 using bit-twiddling from "Hacker's Delight"
pim_inline float VEC_CALL RadicalInverseBase2(u32 bits)
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
    float2 c = { (float)i / (float)N, RadicalInverseBase2(i) };
    return c;
}

pim_inline float3 VEC_CALL SampleUnitSphere(float2 Xi)
{
    float z = Xi.x * 2.0f - 1.0f;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float3 dir = { x, y, z };
    return dir;
}

// samples unit hemisphere with N = (0, 0, 1)
pim_inline float3 VEC_CALL SampleUnitHemisphere(float2 Xi)
{
    float z = Xi.x;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float3 dir = { x, y, z };
    return dir;
}

// returns a microfacet normal of the GGX NDF for given roughness
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

PIM_C_END
