#pragma once

#include "math/float4_funcs.h"
#include "math/int4_funcs.h"

PIM_C_BEGIN

// Interleaved Gradient Noise
//  - Jimenez, Next Generation Post Processing in Call of Duty: Advanced Warfare
//    Advances in Real-time Rendering, SIGGRAPH 2014
pim_inline float VEC_CALL InterleavedGradientNoise(float2 v)
{
    return f1_frac(f1_frac(v.x * 0.06711056f + v.y * 0.00583715f) * 52.9829189f);
}

pim_inline float4 VEC_CALL GradientCell3(int4 c, u32 seed)
{
    u32 i = Pcg4((u32)c.x, (u32)c.y, (u32)c.z, seed).x;
    float4 v;
    v.x = i & (1u << 31) ? 1.0f : -1.0f;
    v.y = i & (1u << 30) ? 1.0f : -1.0f;
    v.z = i & (1u << 29) ? 1.0f : -1.0f;
    v.w = 0.0f;
    return v;
}

pim_inline float VEC_CALL GradientNoise3(float4 p, u32 seed)
{
    int4 i = f4_i4(f4_floor(p));
    float4 f = f4_frac(p);

    float c000 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(0, 0, 0, 0)), seed),
        f4_sub(f, f4_v(0.0f, 0.0f, 0.0f, 0.0f)));
    float c001 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(0, 0, 1, 0)), seed),
        f4_sub(f, f4_v(0.0f, 0.0f, 1.0f, 0.0f)));

    float c010 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(0, 1, 0, 0)), seed),
        f4_sub(f, f4_v(0.0f, 1.0f, 0.0f, 0.0f)));
    float c011 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(0, 1, 1, 0)), seed),
        f4_sub(f, f4_v(0.0f, 1.0f, 1.0f, 0.0f)));

    float c100 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(1, 0, 0, 0)), seed),
        f4_sub(f, f4_v(1.0f, 0.0f, 0.0f, 0.0f)));
    float c101 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(1, 0, 1, 0)), seed),
        f4_sub(f, f4_v(1.0f, 0.0f, 1.0f, 0.0f)));

    float c110 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(1, 1, 0, 0)), seed),
        f4_sub(f, f4_v(1.0f, 1.0f, 0.0f, 0.0f)));
    float c111 = f4_dot3(
        GradientCell3(i4_add(i, i4_v(1, 1, 1, 0)), seed),
        f4_sub(f, f4_v(1.0f, 1.0f, 1.0f, 0.0f)));

    float4 u = f4_unormstep(f);
    float4 c0vz = f4_lerpvs(
        f4_v(c000, c010, c100, c110),
        f4_v(c001, c011, c101, c111),
        u.z);
    float c0yz = f1_lerp(c0vz.x, c0vz.y, u.y);
    float c1yz = f1_lerp(c0vz.z, c0vz.w, u.y);
    return f1_lerp(c0yz, c1yz, u.x);
}

pim_inline float VEC_CALL FbmGradientNoise3(float4 p, float lacunarity, float gain, i32 octaves, u32 seed)
{
    float sum = 0.0f;
    float freq = 1.0f;
    float ampl = 1.0f;
    for (i32 i = 0; i < octaves; ++i)
    {
        float noise = GradientNoise3(f4_mulvs(p, freq), seed + (u32)i + 1u);
        sum += noise * ampl;
        freq *= lacunarity;
        ampl *= gain;
    }
    return sum;
}

PIM_C_END
