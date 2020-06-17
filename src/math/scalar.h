#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdbool.h>
#include <math.h>

#pragma intrinsic(pow, sqrt, exp, log, sin, cos, tan, fmod)

#define kPi                 3.141592653f
#define kTau                6.283185307f
#define kRadiansPerDegree   (kTau / 360.0f)
#define kDegreesPerRadian   (360.0f / kTau)
#define kEpsilon            (1.0f / (1<<20))

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
    return ((x > 0.0f) ? 1.0f : 0.0f) - ((x < 0.0f) ? 1.0f : 0.0f);
}

pim_inline float VEC_CALL f1_clamp(float x, float lo, float hi)
{
    return f1_min(hi, f1_max(lo, x));
}

pim_inline float VEC_CALL f1_saturate(float x)
{
    return f1_clamp(x, 0.0f, 1.0f);
}
#define f1_sat(x) f1_saturate(x)

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

pim_inline float VEC_CALL f1_pow(float x, float n)
{
    // spherical gaussian approximation
    // only worth a damn when n >= 5
    // n = n * 1.4427f + 1.4427f;
    // return exp2f(x * n - n);
    return powf(x, n);
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

pim_inline float VEC_CALL f1_sq(float x)
{
    return x * x;
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

pim_inline i32 VEC_CALL i1_log2(i32 x)
{
    i32 y = -(x == 0);
    if (x >= (1 << 16))
    {
        y += 16;
        x >>= 16;
    }
    if (x >= (1 << 8))
    {
        y += 8;
        x >>= 8;
    }
    if (x >= (1 << 4))
    {
        y += 4;
        x >>= 4;
    }
    if (x >= (1 << 2))
    {
        y += 2;
        x >>= 2;
    }
    if (x >= (1 << 1))
    {
        y += 1;
        x >>= 1;
    }
    return y;
}

PIM_C_END
