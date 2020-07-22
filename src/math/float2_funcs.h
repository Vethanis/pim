#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "common/random.h"

#define f2_0 f2_s(0.0f)
#define f2_1 f2_s(1.0f)
#define f2_2 f2_s(2.0f)
#define f2_rcp2 f2_s(0.5f)
#define f2_rcp3 f2_s(0.33333333f)

pim_inline float2 VEC_CALL f2_v(float x, float y)
{
    return (float2) { x, y };
}

pim_inline float2 VEC_CALL f2_s(float s)
{
    return (float2) { s, s };
}

pim_inline float2 VEC_CALL f2_iv(i32 x, i32 y)
{
    return (float2) { (float)x, (float)y };
}

pim_inline float2 VEC_CALL f2_is(i32 s)
{
    return (float2) { (float)s, (float)s };
}

pim_inline int2 VEC_CALL f2_i2(float2 f)
{
    int2 i = { (i32)f.x, (i32)f.y };
    return i;
}

pim_inline float2 VEC_CALL f2_xx(float2 v)
{
    float2 vec = { v.x, v.x };
    return vec;
}

pim_inline float2 VEC_CALL f2_yy(float2 v)
{
    float2 vec = { v.y, v.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_yx(float2 v)
{
    float2 vec = { v.y, v.x };
    return vec;
}

pim_inline float2 VEC_CALL f2_load(const float* src)
{
    float2 vec = { src[0], src[1] };
    return vec;
}

pim_inline void VEC_CALL f2_store(float2 src, float* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
}

pim_inline float2 VEC_CALL f2_add(float2 lhs, float2 rhs)
{
    float2 vec = { lhs.x + rhs.x, lhs.y + rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_addvs(float2 lhs, float rhs)
{
    float2 vec = { lhs.x + rhs, lhs.y + rhs };
    return vec;
}

pim_inline float2 VEC_CALL f2_addsv(float lhs, float2 rhs)
{
    float2 vec = { lhs + rhs.x, lhs + rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_subvs(float2 lhs, float rhs)
{
    float2 vec = { lhs.x - rhs, lhs.y - rhs };
    return vec;
}

pim_inline float2 VEC_CALL f2_subsv(float lhs, float2 rhs)
{
    float2 vec = { lhs - rhs.x, lhs - rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_sub(float2 lhs, float2 rhs)
{
    float2 vec = { lhs.x - rhs.x, lhs.y - rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_mul(float2 lhs, float2 rhs)
{
    float2 vec = { lhs.x * rhs.x, lhs.y * rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_mulvs(float2 lhs, float rhs)
{
    float2 vec = { lhs.x * rhs, lhs.y * rhs };
    return vec;
}

pim_inline float2 VEC_CALL f2_mulsv(float lhs, float2 rhs)
{
    float2 vec = { lhs * rhs.x, lhs * rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_div(float2 lhs, float2 rhs)
{
    float2 vec = { lhs.x / rhs.x, lhs.y / rhs.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_divvs(float2 lhs, float rhs)
{
    return f2_mulvs(lhs, 1.0f / rhs);
}

pim_inline float2 VEC_CALL f2_neg(float2 v)
{
    float2 vec = { -v.x, -v.y };
    return vec;
}

pim_inline bool2 VEC_CALL f2_eq(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL f2_neq(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL f2_lt(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL f2_gt(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL f2_lteq(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL f2_gteq(float2 lhs, float2 rhs)
{
    bool2 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL b2_not(bool2 b)
{
    bool2 y = { ~b.x, ~b.y };
    return y;
}

pim_inline bool2 VEC_CALL b2_and(bool2 lhs, bool2 rhs)
{
    bool2 vec = { lhs.x & rhs.x, lhs.y & rhs.y };
    return vec;
}

pim_inline bool2 VEC_CALL b2_or(bool2 lhs, bool2 rhs)
{
    bool2 vec = { lhs.x | rhs.x, lhs.y | rhs.y };
    return vec;
}

pim_inline bool2 VEC_CALL b2_xor(bool2 lhs, bool2 rhs)
{
    bool2 vec = { lhs.x ^ rhs.x, lhs.y ^ rhs.y };
    return vec;
}

pim_inline bool2 VEC_CALL b2_nand(bool2 lhs, bool2 rhs)
{
    return b2_not(b2_and(lhs, rhs));
}

pim_inline bool2 VEC_CALL b2_nor(bool2 lhs, bool2 rhs)
{
    return b2_not(b2_or(lhs, rhs));
}

pim_inline bool VEC_CALL b2_any(bool2 b)
{
    return b.x | b.y;
}

pim_inline bool VEC_CALL b2_all(bool2 b)
{
    return b.x & b.y;
}

pim_inline float VEC_CALL f2_sum(float2 v)
{
    return v.x + v.y;
}

pim_inline float2 VEC_CALL f2_rcp(float2 v)
{
    float2 vec = { 1.0f / v.x, 1.0f / v.y };
    return vec;
}

pim_inline float2 VEC_CALL f2_sqrt(float2 v)
{
    float2 vec = { sqrtf(v.x), sqrtf(v.y) };
    return vec;
}

pim_inline float2 VEC_CALL f2_abs(float2 v)
{
    float2 vec = { f1_abs(v.x), f1_abs(v.y) };
    return vec;
}

pim_inline float2 VEC_CALL f2_min(float2 a, float2 b)
{
    float2 vec =
    {
        f1_min(a.x, b.x),
        f1_min(a.y, b.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_max(float2 a, float2 b)
{
    float2 vec =
    {
        f1_max(a.x, b.x),
        f1_max(a.y, b.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_select(float2 a, float2 b, bool2 t)
{
    float2 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
    };
    return c;
}

pim_inline float VEC_CALL f2_hmin(float2 v)
{
    return f1_min(v.x, v.y);
}

pim_inline float VEC_CALL f2_hmax(float2 v)
{
    return f1_max(v.x, v.y);
}

pim_inline float2 VEC_CALL f2_clamp(float2 x, float2 lo, float2 hi)
{
    return f2_min(f2_max(x, lo), hi);
}

pim_inline float VEC_CALL f2_dot(float2 a, float2 b)
{
    return f2_sum(f2_mul(a, b));
}

pim_inline float VEC_CALL f2_length(float2 x)
{
    return sqrtf(f2_dot(x, x));
}

pim_inline float2 VEC_CALL f2_normalize(float2 x)
{
    return f2_mul(x, f2_s(1.0f / f2_length(x)));
}

pim_inline float VEC_CALL f2_distance(float2 a, float2 b)
{
    return f2_length(f2_sub(a, b));
}

pim_inline float VEC_CALL f2_lengthsq(float2 x)
{
    return f2_dot(x, x);
}

pim_inline float VEC_CALL f2_distancesq(float2 a, float2 b)
{
    return f2_lengthsq(f2_sub(a, b));
}

pim_inline float2 VEC_CALL f2_lerp(float2 a, float2 b, float t)
{
    float2 vt = f2_s(t);
    float2 ba = f2_sub(b, a);
    b = f2_mul(ba, vt);
    return f2_add(a, b);
}

pim_inline float2 VEC_CALL f2_saturate(float2 a)
{
    return f2_clamp(a, f2_0, f2_1);
}

pim_inline float2 VEC_CALL f2_step(float2 a, float2 b)
{
    return f2_select(f2_0, f2_1, f2_gteq(a, b));
}

pim_inline float2 VEC_CALL f2_smoothstep(float2 a, float2 b, float2 x)
{
    float2 t = f2_saturate(f2_div(f2_sub(x, a), f2_sub(b, a)));
    float2 s = f2_sub(f2_s(3.0f), f2_mul(f2_2, t));
    return f2_mul(f2_mul(t, t), s);
}

pim_inline float2 VEC_CALL f2_unormstep(float2 t)
{
    float2 s = f2_sub(f2_s(3.0f), f2_mul(f2_2, t));
    return f2_mul(f2_mul(t, t), s);
}

pim_inline float2 VEC_CALL f2_unormerstep(float2 t)
{
    float2 y = { f1_unormerstep(t.x), f1_unormerstep(t.y) };
    return y;
}

pim_inline float2 VEC_CALL f2_reflect(float2 i, float2 n)
{
    float2 nidn = f2_mul(n, f2_s(f2_dot(i, n)));
    return f2_sub(i, f2_mul(f2_2, nidn));
}

pim_inline float2 VEC_CALL f2_refract(float2 i, float2 n, float ior)
{
    float ndi = f2_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    float2 m = f2_sub(
        f2_mul(f2_s(ior), i),
        f2_mul(f2_s(l), n));
    return k >= 0.0f ? f2_0 : m;
}

pim_inline float2 VEC_CALL f2_pow(float2 v, float2 e)
{
    float2 vec =
    {
        f1_pow(v.x, e.x),
        f1_pow(v.y, e.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_exp(float2 v)
{
    float2 vec =
    {
        expf(v.x),
        expf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_log(float2 v)
{
    float2 vec =
    {
        logf(v.x),
        logf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_sin(float2 v)
{
    float2 vec =
    {
        sinf(v.x),
        sinf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_cos(float2 v)
{
    float2 vec =
    {
        cosf(v.x),
        cosf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_tan(float2 v)
{
    float2 vec =
    {
        tanf(v.x),
        tanf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_asin(float2 v)
{
    float2 vec =
    {
        asinf(v.x),
        asinf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_acos(float2 v)
{
    float2 vec =
    {
        acosf(v.x),
        acosf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_atan(float2 v)
{
    float2 vec =
    {
        atanf(v.x),
        atanf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_floor(float2 v)
{
    float2 vec =
    {
        floorf(v.x),
        floorf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_ceil(float2 v)
{
    float2 vec =
    {
        ceilf(v.x),
        ceilf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_trunc(float2 v)
{
    float2 vec =
    {
        truncf(v.x),
        truncf(v.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_frac(float2 v)
{
    return f2_sub(v, f2_floor(v));
}

pim_inline float2 VEC_CALL f2_fmod(float2 a, float2 b)
{
    float2 vec =
    {
        fmodf(a.x, b.x),
        fmodf(a.y, b.y),
    };
    return vec;
}

pim_inline float2 VEC_CALL f2_rad(float2 x)
{
    return f2_mul(x, f2_s(kRadiansPerDegree));
}

pim_inline float2 VEC_CALL f2_deg(float2 x)
{
    return f2_mul(x, f2_s(kDegreesPerRadian));
}

pim_inline float2 VEC_CALL f2_blend(float2 a, float2 b, float2 c, float4 wuvt)
{
    float2 p = f2_mulvs(a, wuvt.x);
    p = f2_add(p, f2_mulvs(b, wuvt.y));
    p = f2_add(p, f2_mulvs(c, wuvt.z));
    return p;
}

pim_inline float2 VEC_CALL f2_rand(prng_t* rng)
{
    return f2_v(prng_f32(rng), prng_f32(rng));
}

pim_inline float2 VEC_CALL f2_snorm(float2 u)
{
    return f2_subvs(f2_mulvs(u, 2.0f), 1.0f);
}

pim_inline float2 VEC_CALL f2_unorm(float2 s)
{
    return f2_addvs(f2_mulvs(s, 0.5f), 0.5f);
}

pim_inline float2 VEC_CALL f2_tent(float2 Xi)
{
    Xi = f2_mulvs(Xi, 2.0f);
    Xi.x = Xi.x < 1.0f ? sqrtf(Xi.x) - 1.0f : 1.0f - sqrtf(2.0f - Xi.x);
    Xi.y = Xi.y < 1.0f ? sqrtf(Xi.y) - 1.0f : 1.0f - sqrtf(2.0f - Xi.x);
    return Xi;
}

PIM_C_END
