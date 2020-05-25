#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/scalar.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"

pim_inline bool VEC_CALL IsUnitLength(float4 dir)
{
    return f1_distance(f4_length3(dir), 1.0f) < f16_eps;
}

pim_inline float3x3 VEC_CALL NormalToTBN(float4 N)
{
    const float4 kX = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float4 kZ = { 0.0f, 0.0f, 1.0f, 0.0f };

    ASSERT(IsUnitLength(N));

    float4 a = f1_abs(N.z) < 0.9f ? kZ : kX;
    float4 T = f4_normalize3(f4_cross3(a, N));
    float4 B = f4_cross3(N, T);

    ASSERT(IsUnitLength(B));

    return (float3x3) { T, B, N };
}

pim_inline float4 VEC_CALL TbnToWorld(float3x3 TBN, float4 nTS)
{
    float4 r = f4_mulvs(TBN.c0, nTS.x);
    float4 u = f4_mulvs(TBN.c1, nTS.y);
    float4 f = f4_mulvs(TBN.c2, nTS.z);
    float4 dir = f4_add(f, f4_add(r, u));
    ASSERT(IsUnitLength(dir));
    return dir;
}

pim_inline float4 VEC_CALL TanToWorld(float4 normalWS, float4 normalTS)
{
    float3x3 TBN = NormalToTBN(normalWS);
    return TbnToWorld(TBN, normalTS);
}

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

// Maps a value in unorm square to a value in snorm circle
// "A Low Distortion Map Between Disk and Square" by Peter Shirley and Kenneth Chiu.
pim_inline float2 VEC_CALL MapSquareToDisk(float2 Xi)
{
    const float kPiD4 = kPi / 4.0f;

    float phi = 0.0f;
    float r = 0.0f;

    float a = 2.0f * Xi.x - 1.0f;
    float b = 2.0f * Xi.y - 1.0f;
    if (a > -b)
    {
        if (a > b)
        {
            r = a;
            phi = kPiD4 * (b / a);
        }
        else
        {
            r = b;
            phi = kPiD4 * (2.0f - (a / b));
        }
    }
    else
    {
        if (a < b)
        {
            r = -a;
            phi = kPiD4 * (4.0f + (b / a));
        }
        else
        {
            r = -b;
            if (b != 0.0f)
            {
                phi = kPiD4 * (6.0f - (a / b));
            }
        }
    }

    return f2_v(r * cosf(phi), r * sinf(phi));
}

// samples unit sphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleUnitSphere(float2 Xi)
{
    float z = Xi.x * 2.0f - 1.0f;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float4 dir = { x, y, z, 0.0f };
    ASSERT(IsUnitLength(dir));
    return dir;
}

// samples unit hemisphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleUnitHemisphere(float2 Xi)
{
    float z = Xi.x;
    float r = sqrtf(f1_max(0.0f, 1.0f - z * z));
    float phi = kTau * Xi.y;
    float x = r * cosf(phi);
    float y = r * sinf(phi);
    float4 dir = { x, y, z, 0.0f };
    ASSERT(IsUnitLength(dir));
    return dir;
}

// samples cosine-weighted hemisphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleCosineHemisphere(float2 Xi)
{
    Xi = MapSquareToDisk(Xi);
    float r = f2_lengthsq(Xi);
    float z = sqrtf(f1_max(0.0f, 1.0f - r));
    float4 dir = { Xi.x, Xi.y, z, 0.0f };
    ASSERT(IsUnitLength(dir));
    return dir;
}

// returns a microfacet normal of the GGX NDF for given roughness
// tangent space
pim_inline float4 VEC_CALL SampleGGXMicrofacet(float2 Xi, float roughness)
{
    float a = roughness * roughness;
    float phi = kTau * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const float4 dir =
    {
        cosf(phi) * sinTheta,
        sinf(phi) * sinTheta,
        cosTheta,
        0.0f,
    };
    ASSERT(IsUnitLength(dir));
    return dir;
}

// world space
pim_inline float4 VEC_CALL SampleGGXDir(float2 Xi, float4 V, float4 N, float roughness)
{
    float4 H = SampleGGXMicrofacet(Xi, roughness);
    H = TanToWorld(N, H);
    float HoV = f1_max(0.0f, f4_dot3(H, V));
    float4 dir = f4_reflect3(f4_neg(V), H);
    ASSERT(IsUnitLength(dir));
    return dir;
}

PIM_C_END
