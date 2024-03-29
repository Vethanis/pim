#pragma once

#include "math/types.h"
#include "math/scalar.h"
#include "math/float2_funcs.h"
#include "math/float4_funcs.h"
#include "math/float3x3_funcs.h"
#include "math/lighting.h"
#include "math/sdf.h"

PIM_C_BEGIN

pim_inline bool VEC_CALL IsUnitLength(float4 dir)
{
    float len = f4_length3(dir);
    float dist = f1_distance(len, 1.0f);
    return dist < 1e-4f;
}

pim_inline bool VEC_CALL IsOrtho(float4 a, float4 b)
{
    float d = f4_dot3(a, b);
    return f1_abs(d) < 1e-4f;
}

pim_inline float3x3 VEC_CALL NormalToTBN(float4 N)
{
    ASSERT(IsUnitLength(N));

    // Building an Orthonormal Basis, Revisited
    // Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler, Max Liani, and Ryusuke Villemin
    // Listing 3
    // https://graphics.pixar.com/library/OrthonormalB/paper.pdf#page=6

    float s = (N.z < 0.0f) ? -1.0f : 1.0f;
    const float a = -1.0f / (s + N.z);
    const float b = N.x * N.y * a;
    float4 b1;
    b1.x = 1.0f + s * N.x * N.x * a;
    b1.y = s * b;
    b1.z = -s * N.x;
    b1.w = 0.0f;
    ASSERT(IsUnitLength(b1));
    ASSERT(IsOrtho(N, b1));

    float4 b2;
    b2.x = b;
    b2.y = s + N.y * N.y * a;
    b2.z = -N.y;
    b2.w = 0.0f;
    ASSERT(IsUnitLength(b2));
    ASSERT(IsOrtho(N, b2));
    ASSERT(IsOrtho(b1, b2));

    float3x3 TBN;
    TBN.c2 = N;
    TBN.c0 = b1;
    TBN.c1 = b2;
    return TBN;
}

pim_inline float4 VEC_CALL TbnToWorld(float3x3 TBN, float4 nTS)
{
    float4 nWS = f3x3_mul_col(TBN, nTS);
    ASSERT(IsUnitLength(nWS));
    return nWS;
}

pim_inline float4 VEC_CALL TanToWorld(float4 normalWS, float4 normalTS)
{
    return TbnToWorld(NormalToTBN(normalWS), normalTS);
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
    float2 c = { (i + 0.5f) / N, RadicalInverseBase2(i) };
    return c;
}

// http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/Importance_Sampling.html
pim_inline float VEC_CALL PowerHeuristic(float f, float g)
{
    return (f * f) / f1_max(f * f + g * g, kEpsilon);
}

// Maps a value in unorm square to a value in snorm circle
// https://psgraphics.blogspot.com/2011/01/improved-code-for-concentric-map.html
pim_inline float2 VEC_CALL MapSquareToDisk(float2 Xi)
{
    Xi = f2_lerpsv(kEpsilon, 1.0f - kEpsilon, Xi);
    float phi, r;
    float a = 2.0f * Xi.x - 1.0f;
    float b = 2.0f * Xi.y - 1.0f;
    if ((a*a) > (b*b))
    {
        r = a;
        phi = (kPi / 4.0f) * (b / a);
    }
    else
    {
        r = b;
        phi = (kPi / 2.0f) - (kPi / 4.0f) * (a / b);
    }

    return f2_v(r * cosf(phi), r * sinf(phi));
}

pim_inline float4 VEC_CALL SampleBaryCoord(float2 Xi)
{
    float r1 = sqrtf(f1_max(Xi.x, kEpsilonSq));
    float r2 = Xi.y;
    float u = r1 * (1.0f - r2);
    float v = r2 * r1;
    float w = 1.0f - (u + v);
    return f4_v(w, u, v, 0.0f);
}

pim_inline float2 VEC_CALL SampleNGon(float2 Xi, u32 side, u32 N, float rot)
{
    side %= N;
    const float R = kTau / N;
    const float a = rot + (1 + side) * R;
    const float b = rot + (2 + side) * R;
    const float2 A = { cosf(a), sinf(a) };
    const float2 B = { cosf(b), sinf(b) };
    return f2_blend(f2_0, A, B, SampleBaryCoord(Xi));
}

pim_inline float2 VEC_CALL SamplePentagram(float2 Xi, u32 side)
{
    // https://mathworld.wolfram.com/Pentagram.html
    // https://www.desmos.com/calculator/ptfwlfemow
    const float R = kTau / 5.0f;
    const float s = kPi * 0.1f;
    const float q = (1.0f - kSqrt5Conj) * 0.5f;
    side %= 5;
    const float a = s + (1.0f + side) * R;
    const float b = s + (1.5f + side) * R;
    const float c = s + (2.0f + side) * R;
    const float2 A = { q * cosf(a), q * sinf(a) };
    const float2 B = { cosf(b), sinf(b) };
    const float2 C = { q * cosf(c), q * sinf(c) };
    return f2_bilerp(A, B, f2_0, C, Xi);
}

pim_inline float4 VEC_CALL SphericalToCartesian(float cosTheta, float phi)
{
    float sinTheta = sqrtf(f1_max(1.0f - cosTheta * cosTheta, 0.0f));
    float x = sinTheta * cosf(phi);
    float y = sinTheta * sinf(phi);
    float4 dir = { x, y, cosTheta, 0.0f };
    return dir;
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

pim_inline float4 VEC_CALL SampleParallaxBox(Box3D box, float4 ro, float4 rd)
{
    float2 nf = isectBox3D(ro, f4_rcp(rd), box);
    if (nf.x < nf.y)
    {
        float t = nf.x > 0.0f ? nf.x : nf.y;
        if (t > 0.0f)
        {
            float4 pt = f4_add(ro, f4_mulvs(rd, t));
            rd = f4_normalize3(f4_sub(pt, ro));
        }
    }
    return rd;
}

pim_inline float4 VEC_CALL SampleParallaxSphere(Sphere sph, float4 ro, float4 rd)
{
    float2 nf = isectSphere3D(ro, rd, sph);
    if (nf.x < nf.y)
    {
        float t = nf.x > 0.0f ? nf.x : nf.y;
        if (t > 0.0f)
        {
            float4 pt = f4_add(ro, f4_mulvs(rd, t));
            rd = f4_normalize3(f4_sub(pt, ro));
        }
    }
    return rd;
}

typedef struct SphereSA
{
    float3x3 Vonb;
    float4 sphere;
    float cosTheta;
    float distance;
} SphereSA;

// https://en.wikipedia.org/wiki/Angular_diameter
pim_inline float VEC_CALL CosAngularRadius(float distanceToCenter, float radius)
{
    float sinTheta = radius / distanceToCenter;
    float cosTheta = sqrtf(f1_max(1.0f - sinTheta * sinTheta, kEpsilonSq));
    return cosTheta;
}

pim_inline float VEC_CALL SolidAngleArea(float cosTheta)
{
    return kTau * (1.0f - cosTheta);
}

pim_inline SphereSA VEC_CALL SphereSA_New(float4 sphere, float4 ro)
{
    float4 rd = f4_sub(sphere, ro);
    float dist = f4_length3(rd);
    dist = f1_max(dist, kCenti);
    rd = f4_divvs(rd, dist);
    float4 V = f4_neg(rd);

    SphereSA sa;
    sa.Vonb = NormalToTBN(V);
    sa.sphere = sphere;
    sa.cosTheta = CosAngularRadius(dist, sphere.w);
    sa.distance = dist;
    return sa;
}

pim_inline float4 VEC_CALL SampleSphereSA(SphereSA sa, float2 Xi)
{
    float cosTheta = 1.0f - Xi.y + Xi.y * sa.cosTheta;
    float phi = kTau * Xi.x;
    float4 dirTs = SphericalToCartesian(cosTheta, phi);
    float4 dirWs = TbnToWorld(sa.Vonb, dirTs);
    float4 ptWs = f4_add(sa.sphere, f4_mulvs(dirWs, sa.sphere.w));
    return ptWs;
}

// samples cosine-weighted hemisphere with N = (0, 0, 1)
pim_inline float4 VEC_CALL SampleCosineHemisphere(float2 Xi)
{
    Xi = MapSquareToDisk(Xi);
    float z = sqrtf(f1_max(1.0f - f2_lengthsq(Xi), kEpsilonSq));
    return f4_v(Xi.x, Xi.y, z, 0.0f);
}

// returns a microfacet normal of the GGX NDF for given roughness
// tangent space
pim_inline float4 VEC_CALL SampleGGXMicrofacet(float2 Xi, float alpha)
{
    float a2 = alpha * alpha;
    float phi = kTau * Xi.x;
    float b = f1_max(1.0f + (a2 - 1.0f) * Xi.y, kEpsilon);
    float cosTheta = sqrtf(f1_max((1.0f - Xi.y) / b, kEpsilonSq));
    return SphericalToCartesian(cosTheta, phi);
}

pim_inline float4 VEC_CALL ImportanceSampleGGX(float4 I, float4 N, float2 Xi, float alpha)
{
    float4 m = TanToWorld(N, SampleGGXMicrofacet(Xi, alpha));
    float4 L = f4_reflect3(I, m);
    return L;
}

pim_inline float4 VEC_CALL ImportanceSampleLambert(float4 N, float2 Xi)
{
    return TanToWorld(N, SampleCosineHemisphere(Xi));
}

pim_inline float VEC_CALL ImportanceSampleLambertPdf(float NoL)
{
    return NoL * (1.0f / kPi);
}

pim_inline float VEC_CALL UniformSampleLambertPdf(void)
{
    return (1.0f / kTau);
}

pim_inline float VEC_CALL GGXPdf(float NoH, float HoV, float alpha)
{
    float d = D_GTR(NoH, alpha);
    return (d * NoH) / f1_max(4.0f * HoV, kEpsilon);
}

// returns pdf of an area light that passes the intersection test
// area: area of the light
// cosTheta: saturate(dot(LNorm, normalize(LPos - SurfPos)))
// distSq: dot(LPos - SurfPos, LPos - SurfPos)
pim_inline float VEC_CALL LightPdf(float area, float cosTheta, float distSq)
{
    float t = cosTheta * area;
    return distSq / f1_max(t, kEpsilon);
}

pim_inline float2 VEC_CALL SampleGaussPixelFilter(float2 Xi, float stddev)
{
    // https://www.desmos.com/calculator/yczmioogdu
    float angle = Xi.x * kTau;
    float radius = f1_gauss_invcdf(Xi.y, 0.0f, stddev);
    Xi.x = cosf(angle);
    Xi.y = sinf(angle);
    return f2_mulvs(Xi, radius);
}

// samples a free path in a given media
// Xi: random variable in [0, 1)
// mfp: mean free path, 1 / extinction coefficient
pim_inline float VEC_CALL SampleFreePath(float Xi, float mfp)
{
    return -logf(f1_max(1.0f - Xi, kEpsilon)) * mfp;
}

// returns probability of a free path at t
// t: ray time
// u: extinction coefficient, or majorant in woodcock sampling
pim_inline float VEC_CALL FreePathPdf(float t, float u)
{
    return u * expf(-t * u);
}

PIM_C_END
