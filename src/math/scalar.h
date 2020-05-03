#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdbool.h>
#include <math.h>

#pragma intrinsic(pow, sqrt, exp, log, sin, cos, tan, fmod)

#ifndef kPi
#define kPi                 3.141592653f
#define kTau                6.283185307f
#define kRadiansPerDegree   (kTau / 360.0f)
#define kDegreesPerRadian   (360.0f / kTau)
#endif // kPi

// https://en.wikipedia.org/wiki/Machine_epsilon
static const float f16_eps = 1.0f / (1 << 10);
static const float f32_eps = 1.0f / (1 << 23);
static const double f64_eps = 1.0 / (1ll << 52);

pim_inline float VEC_CALL f1_radians(float x)
{
    return x * kRadiansPerDegree;
}

pim_inline float VEC_CALL f1_degrees(float x)
{
    return x * kDegreesPerRadian;
}

pim_inline float VEC_CALL f1_unorm(float s)
{
    return 0.5f + 0.5f * s;
}

pim_inline float VEC_CALL f1_snorm(float u)
{
    return 2.0f * u - 1.0f;
}

pim_inline float VEC_CALL f1_min(float a, float b)
{
    return a < b ? a : b;
}

pim_inline float VEC_CALL f1_max(float a, float b)
{
    return a > b ? a : b;
}

pim_inline float VEC_CALL f1_sign(float x)
{
    return ((x > 0.0f) ? 1.0f : 0.0f) - ((x < 0.0f) ? -1.0f : 0.0f);
}

pim_inline float VEC_CALL f1_clamp(float x, float lo, float hi)
{
    return f1_min(hi, f1_max(lo, x));
}

pim_inline float VEC_CALL f1_saturate(float x)
{
    return f1_clamp(x, 0.0f, 1.0f);
}

pim_inline float VEC_CALL f1_abs(float x)
{
    return f1_max(x, -x);
}

pim_inline float VEC_CALL f1_ceil(float x)
{
    return ceilf(x);
}

pim_inline float VEC_CALL f1_floor(float x)
{
    return floorf(x);
}

pim_inline float VEC_CALL f1_round(float x)
{
    return roundf(x);
}

pim_inline float VEC_CALL f1_trunc(float x)
{
    return truncf(x);
}

pim_inline float VEC_CALL f1_frac(float x)
{
    return x - f1_floor(x);
}

pim_inline float VEC_CALL f1_mod(float num, float div)
{
    return fmodf(num, div);
}

pim_inline float VEC_CALL f1_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

pim_inline float VEC_CALL f1_qbezier(float a, float b, float c, float t)
{
    return f1_lerp(f1_lerp(a, b, t), f1_lerp(b, c, t), t);
}

pim_inline float VEC_CALL f1_select(float a, float b, float t)
{
    return (t != 0.0f) ? b : a;
}

pim_inline float VEC_CALL f1_step(float a, float b)
{
    return (a >= b) ? 1.0f : 0.0f;
}

pim_inline float VEC_CALL f1_smoothstep(float a, float b, float x)
{
    float t = f1_saturate((x - a) / (b - a));
    return (t * t) * (3.0f - 2.0f * t);
}

pim_inline float VEC_CALL f1_reflect(float i, float n)
{
    return i - 2.0f * (n * i * n);
}

pim_inline float VEC_CALL f1_distance(float a, float b)
{
    return f1_abs(b - a);
}

pim_inline i32 VEC_CALL i1_min(i32 a, i32 b)
{
    return a < b ? a : b;
}

pim_inline i32 VEC_CALL i1_max(i32 a, i32 b)
{
    return a > b ? a : b;
}

pim_inline i32 VEC_CALL i1_clamp(i32 x, i32 lo, i32 hi)
{
    return i1_min(hi, i1_max(lo, x));
}

pim_inline i32 VEC_CALL i1_abs(i32 x)
{
    return x & 0x7fffffff;
}

pim_inline i32 VEC_CALL i1_lerp(i32 a, i32 b, i32 t)
{
    return a + (b - a) * t;
}

pim_inline i32 VEC_CALL i1_distance(i32 a, i32 b)
{
    return i1_abs(b - a);
}

pim_inline float VEC_CALL f1_tosrgb(float c)
{
    float s1 = sqrtf(c);
    float s2 = sqrtf(s1);
    float s3 = sqrtf(s2);
    return 0.582937f * s1 + 0.788192f * s2 - 0.370654f * s3;
}

pim_inline float VEC_CALL f1_tolinear(float c)
{
    float c2 = c * c;
    float c3 = c2 * c;
    return 0.016047f * c + 0.668073f * c2 + 0.317642f * c3;
}

pim_inline float VEC_CALL tmap1_reinhard(float x)
{
    return x / (1.0f + x);
}

pim_inline float VEC_CALL tmap1_aces(float x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// note: outputs roughly gamma2.2
pim_inline float VEC_CALL tmap1_filmic(float x)
{
    x = f1_max(0.0f, x - 0.004f);
    return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
pim_inline float VEC_CALL tmap1_uchart2(float x)
{
    const float a = 0.15f;
    const float b = 0.5f;
    const float c = 0.1f;
    const float d = 0.2f;
    const float e = 0.02f;
    const float f = 0.3f;
    const float w = 11.2f;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - (e / f);
}

PIM_C_END
