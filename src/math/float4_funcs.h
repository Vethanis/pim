#pragma once

#include "math/float4.h"

PIM_C_BEGIN

#include "math/f32_funcs.h"

#define f4_0 f4_s(0.0f)
#define f4_1 f4_s(1.0f)
#define f4_2 f4_s(2.0f)

math_inline float4 VEC_CALL f4_v(float x, float y, float z, float w)
{
    float4 vec = { x, y, z, w };
    return vec;
}

math_inline float4 VEC_CALL f4_s(float s)
{
    float4 vec = { s, s, s, s };
    return vec;
}

math_inline float4 VEC_CALL f4_load(const float* src)
{
    float4 vec = { src[0], src[1], src[2], src[3] };
    return vec;
}

math_inline void VEC_CALL f4_store(float4 src, float* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
    dst[3] = src.w;
}

math_inline float4 VEC_CALL f4_zxy(float4 v)
{
    float4 vec = { v.z, v.x, v.y, v.w };
    return vec;
}

math_inline float4 VEC_CALL f4_yzx(float4 v)
{
    float4 vec = { v.y, v.z, v.x, v.w };
    return vec;
}

math_inline float4 VEC_CALL f4_neg(float4 v)
{
    float4 vec = { -v.x, -v.y, -v.z, -v.w };
    return vec;
}

math_inline float4 VEC_CALL f4_rcp(float4 v)
{
    float4 vec = { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z, 1.0f / v.w };
    return vec;
}

math_inline float4 VEC_CALL f4_add(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return vec;
}

math_inline float4 VEC_CALL f4_sub(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return vec;
}

math_inline float4 VEC_CALL f4_mul(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
    return vec;
}

math_inline float4 VEC_CALL f4_div(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
    return vec;
}

math_inline float4 VEC_CALL f4_eq(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x == rhs.x ? 1.0f : 0.0f,
        lhs.y == rhs.y ? 1.0f : 0.0f,
        lhs.z == rhs.z ? 1.0f : 0.0f,
        lhs.w == rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float4 VEC_CALL f4_neq(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x != rhs.x ? 1.0f : 0.0f,
        lhs.y != rhs.y ? 1.0f : 0.0f,
        lhs.z != rhs.z ? 1.0f : 0.0f,
        lhs.w != rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float4 VEC_CALL f4_lt(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x < rhs.x ? 1.0f : 0.0f,
        lhs.y < rhs.y ? 1.0f : 0.0f,
        lhs.z < rhs.z ? 1.0f : 0.0f,
        lhs.w < rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float4 VEC_CALL f4_gt(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x > rhs.x ? 1.0f : 0.0f,
        lhs.y > rhs.y ? 1.0f : 0.0f,
        lhs.z > rhs.z ? 1.0f : 0.0f,
        lhs.w > rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float4 VEC_CALL f4_lteq(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x <= rhs.x ? 1.0f : 0.0f,
        lhs.y <= rhs.y ? 1.0f : 0.0f,
        lhs.z <= rhs.z ? 1.0f : 0.0f,
        lhs.w <= rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float4 VEC_CALL f4_gteq(float4 lhs, float4 rhs)
{
    float4 vec =
    {
        lhs.x >= rhs.x ? 1.0f : 0.0f,
        lhs.y >= rhs.y ? 1.0f : 0.0f,
        lhs.z >= rhs.z ? 1.0f : 0.0f,
        lhs.w >= rhs.w ? 1.0f : 0.0f,
    };
    return vec;
}

math_inline float VEC_CALL f4_sum(float4 v)
{
    return v.x + v.y + v.z + v.w;
}

math_inline bool VEC_CALL f4_any(float4 b)
{
    return f4_sum(b) != 0.0f;
}

math_inline bool VEC_CALL f4_all(float4 b)
{
    return f4_sum(b) == 4.0f;
}

math_inline float4 VEC_CALL f4_not(float4 b)
{
    return f4_sub(f4_1, b);
}

math_inline float4 VEC_CALL f4_min(float4 a, float4 b)
{
    float4 vec =
    {
        f32_min(a.x, b.x),
        f32_min(a.y, b.y),
        f32_min(a.z, b.z),
        f32_min(a.w, b.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_max(float4 a, float4 b)
{
    float4 vec =
    {
        f32_max(a.x, b.x),
        f32_max(a.y, b.y),
        f32_max(a.z, b.z),
        f32_max(a.w, b.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_select(float4 a, float4 b, float4 t)
{
    float4 c =
    {
        t.x != 0.0f ? b.x : a.x,
        t.y != 0.0f ? b.y : a.y,
        t.z != 0.0f ? b.z : a.z,
        t.w != 0.0f ? b.w : a.w,
    };
    return c;
}

math_inline float VEC_CALL f4_hmin(float4 v)
{
    float a = f32_min(v.x, v.y);
    float b = f32_min(v.z, v.w);
    return f32_min(a, b);
}

math_inline float VEC_CALL f4_hmax(float4 v)
{
    float a = f32_max(v.x, v.y);
    float b = f32_max(v.z, v.w);
    return f32_max(a, b);
}

math_inline float4 VEC_CALL f4_clamp(float4 x, float4 lo, float4 hi)
{
    return f4_min(f4_max(x, lo), hi);
}

math_inline float VEC_CALL f4_dot(float4 a, float4 b)
{
    return f4_sum(f4_mul(a, b));
}

math_inline float VEC_CALL f4_length(float4 x)
{
    return sqrtf(f4_dot(x, x));
}

math_inline float4 VEC_CALL f4_normalize(float4 x)
{
    return f4_mul(x, f4_s(1.0f / f4_length(x)));
}

math_inline float VEC_CALL f4_distance(float4 a, float4 b)
{
    return f4_length(f4_sub(a, b));
}

math_inline float VEC_CALL f4_lengthsq(float4 x)
{
    return f4_dot(x, x);
}

math_inline float VEC_CALL f4_distancesq(float4 a, float4 b)
{
    return f4_lengthsq(f4_sub(a, b));
}

math_inline float4 VEC_CALL f4_lerp(float4 a, float4 b, float t)
{
    float4 vt = f4_s(t);
    float4 ba = f4_sub(b, a);
    b = f4_mul(ba, vt);
    return f4_add(a, b);
}

math_inline float4 VEC_CALL f4_saturate(float4 a)
{
    return f4_clamp(a, f4_0, f4_1);
}

math_inline float4 VEC_CALL f4_step(float4 a, float4 b)
{
    return f4_select(f4_0, f4_1, f4_gteq(a, b));
}

math_inline float4 VEC_CALL f4_smoothstep(float4 a, float4 b, float4 x)
{
    float4 t = f4_saturate(f4_div(f4_sub(x, a), f4_sub(b, a)));
    float4 s = f4_sub(f4_s(3.0f), f4_mul(f4_2, t));
    return f4_mul(f4_mul(t, t), s);
}

math_inline float4 VEC_CALL f4_reflect(float4 i, float4 n)
{
    float4 nidn = f4_mul(n, f4_s(f4_dot(i, n)));
    return f4_sub(i, f4_mul(f4_2, nidn));
}

math_inline float4 VEC_CALL f4_refract(float4 i, float4 n, float ior)
{
    float ndi = f4_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    float4 m = f4_sub(
        f4_mul(f4_s(ior), i),
        f4_mul(f4_s(l), n));
    return k >= 0.0f ? f4_0 : m;
}

math_inline float4 VEC_CALL f4_sqrt(float4 v)
{
    float4 vec = { sqrtf(v.x), sqrtf(v.y), sqrtf(v.z), sqrtf(v.w) };
    return vec;
}

math_inline float4 VEC_CALL f4_abs(float4 v)
{
    float4 vec = { fabsf(v.x), fabsf(v.y), fabsf(v.z), fabsf(v.w) };
    return vec;
}

math_inline float4 VEC_CALL f4_pow(float4 v, float4 e)
{
    float4 vec =
    {
        powf(v.x, e.x),
        powf(v.y, e.y),
        powf(v.z, e.z),
        powf(v.w, e.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_exp(float4 v)
{
    float4 vec =
    {
        expf(v.x),
        expf(v.y),
        expf(v.z),
        expf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_log(float4 v)
{
    float4 vec =
    {
        logf(v.x),
        logf(v.y),
        logf(v.z),
        logf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_sin(float4 v)
{
    float4 vec =
    {
        sinf(v.x),
        sinf(v.y),
        sinf(v.z),
        sinf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_cos(float4 v)
{
    float4 vec =
    {
        cosf(v.x),
        cosf(v.y),
        cosf(v.z),
        cosf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_tan(float4 v)
{
    float4 vec =
    {
        tanf(v.x),
        tanf(v.y),
        tanf(v.z),
        tanf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_asin(float4 v)
{
    float4 vec =
    {
        asinf(v.x),
        asinf(v.y),
        asinf(v.z),
        asinf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_acos(float4 v)
{
    float4 vec =
    {
        acosf(v.x),
        acosf(v.y),
        acosf(v.z),
        acosf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_atan(float4 v)
{
    float4 vec =
    {
        atanf(v.x),
        atanf(v.y),
        atanf(v.z),
        atanf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_floor(float4 v)
{
    float4 vec =
    {
        floorf(v.x),
        floorf(v.y),
        floorf(v.z),
        floorf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_ceil(float4 v)
{
    float4 vec =
    {
        ceilf(v.x),
        ceilf(v.y),
        ceilf(v.z),
        ceilf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_trunc(float4 v)
{
    float4 vec =
    {
        truncf(v.x),
        truncf(v.y),
        truncf(v.z),
        truncf(v.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_frac(float4 v)
{
    return f4_sub(v, f4_floor(v));
}

math_inline float4 VEC_CALL f4_fmod(float4 a, float4 b)
{
    float4 vec =
    {
        fmodf(a.x, b.x),
        fmodf(a.y, b.y),
        fmodf(a.z, b.z),
        fmodf(a.w, b.w),
    };
    return vec;
}

math_inline float4 VEC_CALL f4_rad(float4 x)
{
    return f4_mul(x, f4_s(kRadiansPerDegree));
}

math_inline float4 VEC_CALL f4_deg(float4 x)
{
    return f4_mul(x, f4_s(kDegreesPerRadian));
}

PIM_C_END
