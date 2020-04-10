#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdbool.h>
#include <math.h>

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

pim_inline float VEC_CALL f32_radians(float x)
{
    return x * kRadiansPerDegree;
}

pim_inline float VEC_CALL f32_degrees(float x)
{
    return x * kDegreesPerRadian;
}

pim_inline float VEC_CALL f32_min(float a, float b)
{
    return a < b ? a : b;
}

pim_inline float VEC_CALL f32_max(float a, float b)
{
    return a > b ? a : b;
}

pim_inline float VEC_CALL f32_clamp(float x, float lo, float hi)
{
    return f32_min(hi, f32_max(lo, x));
}

pim_inline float VEC_CALL f32_saturate(float x)
{
    return f32_clamp(x, 0.0f, 1.0f);
}

pim_inline float VEC_CALL f32_abs(float x)
{
    return fabsf(x);
}

pim_inline float VEC_CALL f32_ceil(float x)
{
    return ceilf(x);
}

pim_inline float VEC_CALL f32_floor(float x)
{
    return floorf(x);
}

pim_inline float VEC_CALL f32_round(float x)
{
    return roundf(x);
}

pim_inline float VEC_CALL f32_trunc(float x)
{
    return truncf(x);
}

pim_inline float VEC_CALL f32_frac(float x)
{
    return x - f32_floor(x);
}

pim_inline float VEC_CALL f32_mod(float num, float div)
{
    return fmodf(num, div);
}

pim_inline float VEC_CALL f32_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

pim_inline float VEC_CALL f32_qbezier(float a, float b, float c, float t)
{
    return f32_lerp(f32_lerp(a, b, t), f32_lerp(b, c, t), t);
}

pim_inline float VEC_CALL f32_select(float a, float b, float t)
{
    return (t != 0.0f) ? b : a;
}

pim_inline float VEC_CALL f32_step(float a, float b)
{
    return (a >= b) ? 1.0f : 0.0f;
}

pim_inline float VEC_CALL f32_smoothstep(float a, float b, float x)
{
    float t = f32_saturate((x - a) / (b - a));
    return (t * t) * (3.0f - 2.0f * t);
}

pim_inline float VEC_CALL f32_reflect(float i, float n)
{
    return i - 2.0f * (n * i * n);
}

pim_inline float VEC_CALL f32_distance(float a, float b)
{
    return f32_abs(b - a);
}

pim_inline i32 VEC_CALL i32_min(i32 a, i32 b)
{
    return a < b ? a : b;
}

pim_inline i32 VEC_CALL i32_max(i32 a, i32 b)
{
    return a > b ? a : b;
}

pim_inline i32 VEC_CALL i32_clamp(i32 x, i32 lo, i32 hi)
{
    return i32_min(hi, i32_max(lo, x));
}

pim_inline i32 VEC_CALL i32_abs(i32 x)
{
    return x & 0x7fffffff;
}

pim_inline i32 VEC_CALL i32_lerp(i32 a, i32 b, i32 t)
{
    return a + (b - a) * t;
}

pim_inline i32 VEC_CALL i32_distance(i32 a, i32 b)
{
    return i32_abs(b - a);
}

PIM_C_END
