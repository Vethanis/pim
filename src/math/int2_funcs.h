#pragma once

#include "math/int2.h"

PIM_C_BEGIN

#include "math/scalar.h"

static const int2 i2_0 = { 0, 0 };
static const int2 i2_1 = { 1, 1 };
static const int2 i2_2 = { 2, 2 };

pim_inline int2 VEC_CALL i2_v(i32 x, i32 y)
{
    int2 vec = { x, y };
    return vec;
}

pim_inline int2 VEC_CALL i2_s(i32 s)
{
    int2 vec = { s, s };
    return vec;
}

pim_inline int2 VEC_CALL i2_fv(float x, float y)
{
    int2 vec = { (i32)x, (i32)y };
    return vec;
}

pim_inline int2 VEC_CALL i2_fs(float s)
{
    int2 vec = { (i32)s, (i32)s };
    return vec;
}

pim_inline int2 VEC_CALL i2_xx(int2 v)
{
    int2 vec = { v.x, v.x };
    return vec;
}

pim_inline int2 VEC_CALL i2_yy(int2 v)
{
    int2 vec = { v.y, v.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_yx(int2 v)
{
    int2 vec = { v.y, v.x };
    return vec;
}

pim_inline int2 VEC_CALL i2_load(const i32* src)
{
    int2 vec = { src[0], src[1] };
    return vec;
}

pim_inline void VEC_CALL i2_store(int2 src, i32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
}

pim_inline int2 VEC_CALL i2_add(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x + rhs.x, lhs.y + rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_sub(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x - rhs.x, lhs.y - rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_mul(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x * rhs.x, lhs.y * rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_div(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x / rhs.x, lhs.y / rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_neg(int2 v)
{
    int2 vec = { -v.x, -v.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_eq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x == rhs.x ? 1 : 0,
        lhs.y == rhs.y ? 1 : 0,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_neq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x != rhs.x ? 1 : 0,
        lhs.y != rhs.y ? 1 : 0,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_lt(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x < rhs.x ? 1.0f : 0.0f,
        lhs.y < rhs.y ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_gt(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x > rhs.x ? 1.0f : 0.0f,
        lhs.y > rhs.y ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_lteq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x <= rhs.x ? 1.0f : 0.0f,
        lhs.y <= rhs.y ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_gteq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x >= rhs.x ? 1.0f : 0.0f,
        lhs.y >= rhs.y ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline i32 VEC_CALL i2_sum(int2 v)
{
    return v.x + v.y;
}

pim_inline bool VEC_CALL i2_any(int2 b)
{
    return i2_sum(b) != 0.0f;
}

pim_inline bool VEC_CALL i2_all(int2 b)
{
    return i2_sum(b) == 2.0f;
}

pim_inline int2 VEC_CALL i2_not(int2 b)
{
    return i2_sub(i2_1, b);
}

pim_inline int2 VEC_CALL i2_rcp(int2 v)
{
    int2 vec = { 1.0f / v.x, 1.0f / v.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_abs(int2 v)
{
    int2 vec = { v.x &= 0x7fffffff, v.y &= 0x7fffffff };
    return vec;
}

pim_inline int2 VEC_CALL i2_min(int2 a, int2 b)
{
    int2 vec =
    {
        i32_min(a.x, b.x),
        i32_min(a.y, b.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_max(int2 a, int2 b)
{
    int2 vec =
    {
        i32_max(a.x, b.x),
        i32_max(a.y, b.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_select(int2 a, int2 b, int2 t)
{
    int2 c =
    {
        t.x != 0.0f ? b.x : a.x,
        t.y != 0.0f ? b.y : a.y,
    };
    return c;
}

pim_inline i32 VEC_CALL i2_hmin(int2 v)
{
    return i32_min(v.x, v.y);
}

pim_inline i32 VEC_CALL i2_hmax(int2 v)
{
    return i32_max(v.x, v.y);
}

pim_inline int2 VEC_CALL i2_clamp(int2 x, int2 lo, int2 hi)
{
    return i2_min(i2_max(x, lo), hi);
}

pim_inline i32 VEC_CALL i2_dot(int2 a, int2 b)
{
    return i2_sum(i2_mul(a, b));
}

pim_inline i32 VEC_CALL i2_length(int2 x)
{
    return sqrtf(i2_dot(x, x));
}

pim_inline int2 VEC_CALL i2_normalize(int2 x)
{
    return i2_mul(x, i2_s(1.0f / i2_length(x)));
}

pim_inline i32 VEC_CALL i2_distance(int2 a, int2 b)
{
    return i2_length(i2_sub(a, b));
}

pim_inline i32 VEC_CALL i2_lengthsq(int2 x)
{
    return i2_dot(x, x);
}

pim_inline i32 VEC_CALL i2_distancesq(int2 a, int2 b)
{
    return i2_lengthsq(i2_sub(a, b));
}

pim_inline int2 VEC_CALL i2_lerp(int2 a, int2 b, i32 t)
{
    int2 vt = i2_s(t);
    int2 ba = i2_sub(b, a);
    b = i2_mul(ba, vt);
    return i2_add(a, b);
}

pim_inline int2 VEC_CALL i2_saturate(int2 a)
{
    return i2_clamp(a, i2_0, i2_1);
}

pim_inline int2 VEC_CALL i2_step(int2 a, int2 b)
{
    return i2_select(i2_0, i2_1, i2_gteq(a, b));
}

pim_inline int2 VEC_CALL i2_smoothstep(int2 a, int2 b, int2 x)
{
    int2 t = i2_saturate(i2_div(i2_sub(x, a), i2_sub(b, a)));
    int2 s = i2_sub(i2_s(3.0f), i2_mul(i2_2, t));
    return i2_mul(i2_mul(t, t), s);
}

pim_inline int2 VEC_CALL i2_reflect(int2 i, int2 n)
{
    int2 nidn = i2_mul(n, i2_s(i2_dot(i, n)));
    return i2_sub(i, i2_mul(i2_2, nidn));
}

pim_inline int2 VEC_CALL i2_refract(int2 i, int2 n, i32 ior)
{
    i32 ndi = i2_dot(n, i);
    i32 k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    i32 l = ior * ndi + sqrtf(k);
    int2 m = i2_sub(
        i2_mul(i2_s(ior), i),
        i2_mul(i2_s(l), n));
    return k >= 0.0f ? i2_0 : m;
}

pim_inline int2 VEC_CALL i2_pow(int2 v, int2 e)
{
    int2 vec =
    {
        powf(v.x, e.x),
        powf(v.y, e.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_exp(int2 v)
{
    int2 vec =
    {
        expf(v.x),
        expf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_log(int2 v)
{
    int2 vec =
    {
        logf(v.x),
        logf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_sin(int2 v)
{
    int2 vec =
    {
        sinf(v.x),
        sinf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_cos(int2 v)
{
    int2 vec =
    {
        cosf(v.x),
        cosf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_tan(int2 v)
{
    int2 vec =
    {
        tanf(v.x),
        tanf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_asin(int2 v)
{
    int2 vec =
    {
        asinf(v.x),
        asinf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_acos(int2 v)
{
    int2 vec =
    {
        acosf(v.x),
        acosf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_atan(int2 v)
{
    int2 vec =
    {
        atanf(v.x),
        atanf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_floor(int2 v)
{
    int2 vec =
    {
        floorf(v.x),
        floorf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_ceil(int2 v)
{
    int2 vec =
    {
        ceilf(v.x),
        ceilf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_trunc(int2 v)
{
    int2 vec =
    {
        truncf(v.x),
        truncf(v.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_frac(int2 v)
{
    return i2_sub(v, i2_floor(v));
}

pim_inline int2 VEC_CALL i2_fmod(int2 a, int2 b)
{
    int2 vec =
    {
        fmodf(a.x, b.x),
        fmodf(a.y, b.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_rad(int2 x)
{
    return i2_mul(x, i2_s(kRadiansPerDegree));
}

pim_inline int2 VEC_CALL i2_deg(int2 x)
{
    return i2_mul(x, i2_s(kDegreesPerRadian));
}

PIM_C_END
