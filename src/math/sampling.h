#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/lighting.h"

PIM_C_BEGIN

static bool VEC_CALL IsUnitLength(float4 dir)
{
    float len = f4_length3(dir);
    float dist = f1_distance(len, 1.0f);
    // kEpsilon is a bit too strict in this case
    if (dist > 0.0001f)
    {
        INTERRUPT();
        return false;
    }
    return true;
}

pim_inline float3x3 VEC_CALL NormalToTBN(float4 N)
{
    ASSERT(IsUnitLength(N));

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

// http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/Importance_Sampling.html
pim_inline float VEC_CALL PowerHeuristic(
    i32 fCt, float fPdf,
    i32 gCt, float gPdf)
{
    float f = fCt * fPdf;
    float g = gCt * gPdf;
    return (f * f) / (f * f + g * g);
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

pim_inline float4 VEC_CALL SphericalToCartesian(float cosTheta, float phi)
{
    float sinTheta = sqrtf(f1_max(0.0f, 1.0f - cosTheta * cosTheta));
    float x = sinTheta * cosf(phi);
    float y = sinTheta * sinf(phi);
    float4 dir = { x, y, cosTheta, 0.0f };
    return f4_normalize3(dir);
}

// samples unit sphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleUnitSphere(float2 Xi)
{
    float phi = kTau * Xi.x;
    float cosTheta = Xi.y * 2.0f - 1.0f;
    return SphericalToCartesian(cosTheta, phi);
}

// samples unit hemisphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleUnitHemisphere(float2 Xi)
{
    float phi = kTau * Xi.x;
    float cosTheta = Xi.y;
    return SphericalToCartesian(cosTheta, phi);
}

pim_inline float4 VEC_CALL SamplePointOnSphere(
    float2 Xi,
    float4 center,
    float radius)
{
    float4 N = SampleUnitSphere(Xi);
    float4 P = f4_add(center, f4_mulvs(N, radius));
    return P;
}

pim_inline float4 VEC_CALL SampleSphereSolidAngle(
    float2 Xi,
    float4 center,
    float radius,
    float4 ro)
{
    float4 rd = f4_sub(center, ro);
    float dist = f4_length3(rd);
    dist = f1_max(dist, kCentimeter);
    rd = f4_divvs(rd, dist);
    float sinTheta0 = radius / dist;
    float cosTheta0 = sqrtf(f1_max(0.0f, 1.0f - sinTheta0 * sinTheta0));

    float cosTheta1 = 1.0f - Xi.y + Xi.y * cosTheta0;
    float phi = kTau * Xi.x;
    float4 dirTs = SphericalToCartesian(cosTheta1, phi);
    float4 dirWs = TanToWorld(f4_neg(rd), dirTs);
    float4 ptWs = f4_add(center, f4_mulvs(dirWs, radius));
    float pdf = 1.0f / (kTau * (1.0f - cosTheta0));
    ptWs.w = pdf;
    return ptWs;
}

// samples cosine-weighted hemisphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleCosineHemisphere(float2 Xi)
{
    Xi = MapSquareToDisk(Xi);
    float r = f2_lengthsq(Xi);
    float z = sqrtf(f1_max(0.0f, 1.0f - r));
    float4 dir = { Xi.x, Xi.y, z, 0.0f };
    return f4_normalize3(dir);
}

// returns a microfacet normal of the GGX NDF for given roughness
// tangent space
pim_inline float4 VEC_CALL SampleGGXMicrofacet(float2 Xi, float alpha)
{
    float a2 = alpha * alpha;
    float phi = kTau * Xi.x;
    float cosTheta = sqrtf((1.0f - Xi.y) / (1.0f + (a2 - 1.0f) * Xi.y));
    return SphericalToCartesian(cosTheta, phi);
}

pim_inline float VEC_CALL LambertPdf(float NoL)
{
    return NoL / kPi;
}

pim_inline float VEC_CALL GGXPdf(float NoH, float HoV, float alpha)
{
    float d = D_GTR(NoH, alpha);
    return (d * NoH) / f1_max(kEpsilon, 4.0f * HoV);
}

// returns pdf of an area light that passes the intersection test
// area: area of the light
// cosTheta: saturate(dot(LNorm, normalize(LPos - SurfPos)))
// distSq: dot(LPos - SurfPos, LPos - SurfPos)
pim_inline float VEC_CALL LightPdf(float area, float cosTheta, float distSq)
{
    return distSq / f1_max(kMinDenom, cosTheta * area);
}

PIM_C_END
