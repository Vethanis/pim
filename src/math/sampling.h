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

// tangent space
pim_inline float4 VEC_CALL SampleUnitSphere(float2 Xi)
{
    float z = Xi.x * 2.0f - 1.0f;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float4 dir = { x, y, z, 0.0f };
    return dir;
}

// samples unit hemisphere with N = (0, 0, 1)
// tangent space
pim_inline float4 VEC_CALL SampleUnitHemisphere(float2 Xi)
{
    float z = Xi.x;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float4 dir = { x, y, z, 0.0f };
    return dir;
}

// returns a microfacet normal of the GGX NDF for given roughness
// tangent space
pim_inline float4 VEC_CALL ImportanceSampleGGX(float2 Xi, float roughness)
{
    float a = roughness * roughness;
    float phi = kTau * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const float4 H =
    {
        cosf(phi) * sinTheta,
        sinf(phi) * sinTheta,
        cosTheta,
        0.0f,
    };
    // TODO: might not need normalized
    return f4_normalize3(H);
}

pim_inline float3x3 VEC_CALL NormalToTBN(float4 N)
{
    const float4 kX = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float4 kZ = { 0.0f, 0.0f, 1.0f, 0.0f };
    float4 up = f1_abs(N.z) < 0.99f ? kZ : kX;
    float4 T = f4_normalize3(f4_cross3(up, N));
    float4 B = f4_normalize3(f4_cross3(N, T));
    return (float3x3) { T, B, N };
}

pim_inline float4 VEC_CALL TbnToWorld(float3x3 TBN, float4 nTS)
{
    float4 right = f4_mulvs(TBN.c0, nTS.x);
    float4 up = f4_mulvs(TBN.c1, nTS.y);
    float4 fwd = f4_mulvs(TBN.c2, nTS.z);
    return f4_normalize3(f4_add(fwd, f4_add(right, up)));
}

pim_inline float4 VEC_CALL TanToWorld(float4 normalWS, float4 normalTS)
{
    float3x3 TBN = NormalToTBN(normalWS);
    return TbnToWorld(TBN, normalTS);
}


PIM_C_END
