#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"

static const float3 f3_0 = { 0.0f, 0.0f, 0.0f };
static const float3 f3_1 = { 1.0f, 1.0f, 1.0f };
static const float3 f3_2 = { 2.0f, 2.0f, 2.0f };
static const float3 f3_rcp2 = { 0.5f, 0.5f, 0.5f };
static const float3 f3_rcp3 = { 0.33333333f, 0.33333333f, 0.33333333f };

pim_inline float3 VEC_CALL f3_v(float x, float y, float z)
{
    float3 vec = { x, y, z };
    return vec;
}

pim_inline float3 VEC_CALL f3_s(float s)
{
    float3 vec = { s, s, s };
    return vec;
}

pim_inline float3 VEC_CALL f3_load(const float* src)
{
    float3 vec = { src[0], src[1], src[2] };
    return vec;
}

pim_inline void VEC_CALL f3_store(float3 src, float* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
}

pim_inline float3 VEC_CALL f3_zxy(float3 v)
{
    float3 vec = { v.z, v.x, v.y };
    return vec;
}

pim_inline float3 VEC_CALL f3_yzx(float3 v)
{
    float3 vec = { v.y, v.z, v.x };
    return vec;
}

pim_inline float3 VEC_CALL f3_neg(float3 v)
{
    float3 vec = { -v.x, -v.y, -v.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_rcp(float3 v)
{
    float3 vec = { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_add(float3 lhs, float3 rhs)
{
    float3 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_addvs(float3 lhs, float rhs)
{
    float3 vec = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs };
    return vec;
}

pim_inline float3 VEC_CALL f3_addsv(float lhs, float3 rhs)
{
    float3 vec = { lhs + rhs.x, lhs + rhs.y, lhs + rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_sub(float3 lhs, float3 rhs)
{
    float3 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_subvs(float3 lhs, float rhs)
{
    float3 vec = { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs };
    return vec;
}

pim_inline float3 VEC_CALL f3_subsv(float lhs, float3 rhs)
{
    float3 vec = { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_mul(float3 lhs, float3 rhs)
{
    float3 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_mulvs(float3 lhs, float rhs)
{
    float3 vec = { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
    return vec;
}

pim_inline float3 VEC_CALL f3_mulsv(float lhs, float3 rhs)
{
    float3 vec = { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_div(float3 lhs, float3 rhs)
{
    float3 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_divvs(float3 lhs, float rhs)
{
    return f3_mulvs(lhs, 1.0f / rhs);
}

pim_inline float3 VEC_CALL f3_divsv(float lhs, float3 rhs)
{
    float3 vec = { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z };
    return vec;
}

pim_inline float3 VEC_CALL f3_eq(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x == rhs.x ? 1.0f : 0.0f,
        lhs.y == rhs.y ? 1.0f : 0.0f,
        lhs.z == rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_neq(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x != rhs.x ? 1.0f : 0.0f,
        lhs.y != rhs.y ? 1.0f : 0.0f,
        lhs.z != rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_lt(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x < rhs.x ? 1.0f : 0.0f,
        lhs.y < rhs.y ? 1.0f : 0.0f,
        lhs.z < rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_ltvs(float3 lhs, float rhs)
{
    float3 vec =
    {
        lhs.x < rhs ? 1.0f : 0.0f,
        lhs.y < rhs ? 1.0f : 0.0f,
        lhs.z < rhs ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_ltsv(float lhs, float3 rhs)
{
    float3 vec =
    {
        lhs < rhs.x ? 1.0f : 0.0f,
        lhs < rhs.y ? 1.0f : 0.0f,
        lhs < rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_gt(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x > rhs.x ? 1.0f : 0.0f,
        lhs.y > rhs.y ? 1.0f : 0.0f,
        lhs.z > rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_gtvs(float3 lhs, float rhs)
{
    float3 vec =
    {
        lhs.x > rhs ? 1.0f : 0.0f,
        lhs.y > rhs ? 1.0f : 0.0f,
        lhs.z > rhs ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_gtsv(float lhs, float3 rhs)
{
    float3 vec =
    {
        lhs > rhs.x ? 1.0f : 0.0f,
        lhs > rhs.y ? 1.0f : 0.0f,
        lhs > rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_lteq(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x <= rhs.x ? 1.0f : 0.0f,
        lhs.y <= rhs.y ? 1.0f : 0.0f,
        lhs.z <= rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_gteq(float3 lhs, float3 rhs)
{
    float3 vec =
    {
        lhs.x >= rhs.x ? 1.0f : 0.0f,
        lhs.y >= rhs.y ? 1.0f : 0.0f,
        lhs.z >= rhs.z ? 1.0f : 0.0f,
    };
    return vec;
}

pim_inline float VEC_CALL f3_sum(float3 v)
{
    return v.x + v.y + v.z;
}

pim_inline bool VEC_CALL f3_any(float3 v)
{
    return f3_sum(v) != 0.0f;
}

pim_inline bool VEC_CALL f3_all(float3 v)
{
    return f3_sum(v) == 3.0f;
}

pim_inline float3 VEC_CALL f3_min(float3 a, float3 b)
{
    float3 vec =
    {
        f1_min(a.x, b.x),
        f1_min(a.y, b.y),
        f1_min(a.z, b.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_max(float3 a, float3 b)
{
    float3 vec =
    {
        f1_max(a.x, b.x),
        f1_max(a.y, b.y),
        f1_max(a.z, b.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_select(float3 a, float3 b, float3 t)
{
    float3 c =
    {
        t.x != 0.0f ? b.x : a.x,
        t.y != 0.0f ? b.y : a.y,
        t.z != 0.0f ? b.z : a.z,
    };
    return c;
}

pim_inline float VEC_CALL f3_hmin(float3 v)
{
    return f1_min(v.x, f1_min(v.y, v.z));
}

pim_inline float VEC_CALL f3_hmax(float3 v)
{
    return f1_max(v.x, f1_max(v.y, v.z));
}

pim_inline float3 VEC_CALL f3_cross(float3 a, float3 b)
{
    return f3_zxy(
        f3_sub(
            f3_mul(f3_zxy(a), b),
            f3_mul(a, f3_zxy(b))));
}

pim_inline float3 VEC_CALL f3_clamp(float3 x, float3 lo, float3 hi)
{
    return f3_min(f3_max(x, lo), hi);
}

pim_inline float3 VEC_CALL f3_saturate(float3 a)
{
    return f3_clamp(a, f3_0, f3_1);
}

pim_inline float VEC_CALL f3_dot(float3 a, float3 b)
{
    return f3_sum(f3_mul(a, b));
}

pim_inline float VEC_CALL f3_length(float3 x)
{
    return sqrtf(f3_dot(x, x));
}

pim_inline float3 VEC_CALL f3_normalize(float3 x)
{
    return f3_mul(x, f3_s(1.0f / f3_length(x)));
}

pim_inline float VEC_CALL f3_distance(float3 a, float3 b)
{
    return f3_length(f3_sub(a, b));
}

pim_inline float VEC_CALL f3_lengthsq(float3 x)
{
    return f3_dot(x, x);
}

pim_inline float VEC_CALL f3_distancesq(float3 a, float3 b)
{
    return f3_lengthsq(f3_sub(a, b));
}

pim_inline float3 VEC_CALL f3_lerp(float3 a, float3 b, float t)
{
    float3 vt = f3_s(t);
    float3 ba = f3_sub(b, a);
    b = f3_mul(ba, vt);
    return f3_add(a, b);
}

pim_inline float3 VEC_CALL f3_step(float3 a, float3 b)
{
    return f3_select(f3_0, f3_1, f3_gteq(a, b));
}

pim_inline float3 VEC_CALL f3_smoothstep(float3 a, float3 b, float3 x)
{
    float3 t = f3_saturate(f3_div(f3_sub(x, a), f3_sub(b, a)));
    float3 s = f3_sub(f3_s(3.0f), f3_mul(f3_2, t));
    return f3_mul(f3_mul(t, t), s);
}

pim_inline float3 VEC_CALL f3_reflect(float3 i, float3 n)
{
    float3 nidn = f3_mul(n, f3_s(f3_dot(i, n)));
    return f3_sub(i, f3_mul(f3_2, nidn));
}

pim_inline float3 VEC_CALL f3_refract(float3 i, float3 n, float ior)
{
    float ndi = f3_dot(n, i);
    float k = 1.0f - ior * ior * (1.0f - ndi * ndi);
    float l = ior * ndi + sqrtf(k);
    float3 m = f3_sub(
        f3_mul(f3_s(ior), i),
        f3_mul(f3_s(l), n));
    return k >= 0.0f ? f3_0 : m;
}

pim_inline float3 VEC_CALL f3_sqrt(float3 v)
{
    float3 vec = { sqrtf(v.x), sqrtf(v.y), sqrtf(v.z) };
    return vec;
}

pim_inline float3 VEC_CALL f3_abs(float3 v)
{
    float3 vec = { fabsf(v.x), fabsf(v.y), fabsf(v.z) };
    return vec;
}

pim_inline float3 VEC_CALL f3_pow(float3 v, float3 e)
{
    float3 vec =
    {
        powf(v.x, e.x),
        powf(v.y, e.y),
        powf(v.z, e.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_exp(float3 v)
{
    float3 vec =
    {
        expf(v.x),
        expf(v.y),
        expf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_log(float3 v)
{
    float3 vec =
    {
        logf(v.x),
        logf(v.y),
        logf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_sin(float3 v)
{
    float3 vec =
    {
        sinf(v.x),
        sinf(v.y),
        sinf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_cos(float3 v)
{
    float3 vec =
    {
        cosf(v.x),
        cosf(v.y),
        cosf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_tan(float3 v)
{
    float3 vec =
    {
        tanf(v.x),
        tanf(v.y),
        tanf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_asin(float3 v)
{
    float3 vec =
    {
        asinf(v.x),
        asinf(v.y),
        asinf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_acos(float3 v)
{
    float3 vec =
    {
        acosf(v.x),
        acosf(v.y),
        acosf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_atan(float3 v)
{
    float3 vec =
    {
        atanf(v.x),
        atanf(v.y),
        atanf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_floor(float3 v)
{
    float3 vec =
    {
        floorf(v.x),
        floorf(v.y),
        floorf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_ceil(float3 v)
{
    float3 vec =
    {
        ceilf(v.x),
        ceilf(v.y),
        ceilf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_trunc(float3 v)
{
    float3 vec =
    {
        truncf(v.x),
        truncf(v.y),
        truncf(v.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_frac(float3 v)
{
    return f3_sub(v, f3_floor(v));
}

pim_inline float3 VEC_CALL f3_fmod(float3 a, float3 b)
{
    float3 vec =
    {
        fmodf(a.x, b.x),
        fmodf(a.y, b.y),
        fmodf(a.z, b.z),
    };
    return vec;
}

pim_inline float3 VEC_CALL f3_rad(float3 x)
{
    return f3_mul(x, f3_s(kRadiansPerDegree));
}

pim_inline float3 VEC_CALL f3_deg(float3 x)
{
    return f3_mul(x, f3_s(kDegreesPerRadian));
}

pim_inline float3 VEC_CALL f3_blend(float3 a, float3 b, float3 c, float3 wuv)
{
    float3 p = f3_mulvs(a, wuv.x);
    p = f3_add(p, f3_mulvs(b, wuv.y));
    p = f3_add(p, f3_mulvs(c, wuv.z));
    return p;
}

pim_inline bool VEC_CALL f3_cliptest(float3 x, float lo, float hi)
{
    return f3_all(f3_ltvs(x, lo)) || f3_all(f3_gtvs(x, hi));
}

PIM_C_END
