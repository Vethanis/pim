#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/lighting.h"

PIM_C_BEGIN

pim_inline bool VEC_CALL IsUnitLength(float4 dir)
{
    return f1_distance(f4_length3(dir), 1.0f) < kEpsilon;
}

pim_inline float3x3 VEC_CALL NormalToTBN(float4 N)
{
    const float4 kX = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float4 kZ = { 0.0f, 0.0f, 1.0f, 0.0f };

    float4 a = f1_abs(N.z) < 0.9f ? kZ : kX;
    float4 T = f4_normalize3(f4_cross3(a, N));
    float4 B = f4_normalize3(f4_cross3(N, T));

    return (float3x3) { T, B, N };
}

pim_inline float4 VEC_CALL TbnToWorld(float3x3 TBN, float4 nTS)
{
    float4 r = f4_mulvs(TBN.c0, nTS.x);
    float4 u = f4_mulvs(TBN.c1, nTS.y);
    float4 f = f4_mulvs(TBN.c2, nTS.z);
    float4 dir = f4_add(f, f4_add(r, u));
    return f4_normalize3(dir);
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

pim_inline float VEC_CALL LambertPdf(float4 N, float4 dir)
{
    return f1_saturate(f4_dot3(N, dir)) / kPi;
}

pim_inline float4 VEC_CALL ScatterHemisphere(prng_t* rng, float4 N)
{
    return TanToWorld(N, SampleUnitHemisphere(f2_rand(rng)));
}

pim_inline float4 VEC_CALL ScatterCosine(prng_t* rng, float4 N)
{
    return TanToWorld(N, SampleCosineHemisphere(f2_rand(rng)));
}

// returns a microfacet normal of the GGX NDF for given roughness
// tangent space
pim_inline float4 VEC_CALL SampleGGXMicrofacet(float2 Xi, float alpha)
{
    float theta = atan2f(alpha * sqrtf(Xi.x), sqrtf(1.0f - Xi.x));
    float phi = kTau * Xi.y;

    float sinTheta = sinf(theta);
    float cosTheta = cosf(theta);
    float sinPhi = sinf(phi);
    float cosPhi = cosf(phi);

    float4 m;
    m.x = sinTheta * cosPhi;
    m.y = cosTheta * sinPhi;
    m.z = cosTheta;
    m.w = 0.0f;

    return f4_normalize3(m);
}

pim_inline float VEC_CALL GGXPdf(float NoH, float HoV, float alpha)
{
    float d = D_GTR(NoH, alpha);
    return (d * NoH) / f1_max(kEpsilon, 4.0f * HoV);
}

pim_inline float4 VEC_CALL ScatterGGX(
    prng_t* rng,
    float4 I,
    float4 N,
    float alpha)
{
    float4 H = SampleGGXMicrofacet(f2_rand(rng), alpha);
    H = TanToWorld(N, H);
    return f4_normalize3(f4_reflect3(I, H));
}

PIM_C_END
