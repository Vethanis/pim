#pragma once

#include "math/types.h"
#include "math/scalar.h"
#include "common/random.h"

PIM_C_BEGIN

#define f4_0        f4_s(0.0f)
#define f4_1        f4_s(1.0f)
#define f4_2        f4_s(2.0f)
#define f4_rcp2     f4_s(0.5f)
#define f4_rcp3     f4_s(0.33333333f)
#define f4_x        f4_v(1.0f, 0.0f, 0.0f, 0.0f)
#define f4_y        f4_v(0.0f, 1.0f, 0.0f, 0.0f)
#define f4_z        f4_v(0.0f, 0.0f, 1.0f, 0.0f)
#define f4_w        f4_v(0.0f, 0.0f, 0.0f, 1.0f)

pim_inline bool VEC_CALL f4_isfinite3(float4 x)
{
    return isfinite(x.x) && isfinite(x.y) && isfinite(x.z);
}

pim_inline float4 VEC_CALL f4_v(float x, float y, float z, float w)
{
    float4 vec = { x, y, z, w };
    return vec;
}

pim_inline float4 VEC_CALL f4_s(float s)
{
    float4 vec = { s, s, s, s };
    return vec;
}

pim_inline int4 VEC_CALL f4_i4(float4 v)
{
    int4 i4 = { (i32)v.x, (i32)v.y, (i32)v.z, (i32)v.w };
    return i4;
}

pim_inline float3 VEC_CALL f4_f3(float4 v)
{
    float3 v3 = { v.x, v.y, v.z };
    return v3;
}

pim_inline float2 VEC_CALL f4_f2(float4 v)
{
    float2 v2 = { v.x, v.y };
    return v2;
}

pim_inline float4 VEC_CALL f4_load(const float* src)
{
    float4 vec = { src[0], src[1], src[2], src[3] };
    return vec;
}

pim_inline void VEC_CALL f4_store(float4 src, float* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
    dst[3] = src.w;
}

pim_inline float VEC_CALL f4_get(float4 v, i32 i)
{
    float y = 0.0f;
    switch (i & 3)
    {
    case 0:
        y = v.x;
        break;
    case 1:
        y = v.y;
        break;
    case 2:
        y = v.z;
        break;
    case 3:
        y = v.w;
        break;
    }
    return y;
}

pim_inline float4 VEC_CALL f4_set(float4 v, i32 i, float x)
{
    switch (i & 3)
    {
    case 0:
        v.x = x;
        break;
    case 1:
        v.y = x;
        break;
    case 2:
        v.z = x;
        break;
    case 3:
        v.w = x;
        break;
    }
    return v;
}

pim_inline float4 VEC_CALL f4_zxy(float4 v)
{
    float4 vec = { v.z, v.x, v.y, 0.0f };
    return vec;
}

pim_inline float4 VEC_CALL f4_yzx(float4 v)
{
    float4 vec = { v.y, v.z, v.x, 0.0f };
    return vec;
}

pim_inline float4 VEC_CALL f4_neg(float4 v)
{
    float4 vec = { -v.x, -v.y, -v.z, -v.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_rcp(float4 v)
{
    float4 vec = { 1.0f / v.x, 1.0f / v.y, 1.0f / v.z, 1.0f / v.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_add(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_addvs(float4 lhs, float rhs)
{
    float4 vec = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs };
    return vec;
}

pim_inline float4 VEC_CALL f4_addsv(float lhs, float4 rhs)
{
    float4 vec = { lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_sub(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_subvs(float4 lhs, float rhs)
{
    float4 vec = { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs };
    return vec;
}

pim_inline float4 VEC_CALL f4_subsv(float lhs, float4 rhs)
{
    float4 vec = { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_mul(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_mulvs(float4 lhs, float rhs)
{
    float4 vec = { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
    return vec;
}

pim_inline float4 VEC_CALL f4_div(float4 lhs, float4 rhs)
{
    float4 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_divvs(float4 lhs, float rhs)
{
    return f4_mulvs(lhs, 1.0f / rhs);
}

pim_inline float4 VEC_CALL f4_divsv(float lhs, float4 rhs)
{
    float4 vec = { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w };
    return vec;
}

pim_inline float4 VEC_CALL f4_inv(float4 x)
{
    return f4_sub(f4_1, x);
}

pim_inline bool4 VEC_CALL f4_eq(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
        lhs.z == rhs.z,
        lhs.w == rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_eqvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x == rhs,
        lhs.y == rhs,
        lhs.z == rhs,
        lhs.w == rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_neq(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
        lhs.z != rhs.z,
        lhs.w != rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_neqvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x != rhs,
        lhs.y != rhs,
        lhs.z != rhs,
        lhs.w != rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_lt(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
        lhs.z < rhs.z,
        lhs.w < rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_ltvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x < rhs,
        lhs.y < rhs,
        lhs.z < rhs,
        lhs.w < rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_ltsv(float lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs < rhs.x,
        lhs < rhs.y,
        lhs < rhs.z,
        lhs < rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gt(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
        lhs.z > rhs.z,
        lhs.w > rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gtvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x > rhs,
        lhs.y > rhs,
        lhs.z > rhs,
        lhs.w > rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gtsv(float lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs > rhs.x,
        lhs > rhs.y,
        lhs > rhs.z,
        lhs > rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_lteq(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
        lhs.z <= rhs.z,
        lhs.w <= rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_lteqvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x <= rhs,
        lhs.y <= rhs,
        lhs.z <= rhs,
        lhs.w <= rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_lteqsv(float lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs <= rhs.x,
        lhs <= rhs.y,
        lhs <= rhs.z,
        lhs <= rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gteq(float4 lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
        lhs.z >= rhs.z,
        lhs.w >= rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gteqvs(float4 lhs, float rhs)
{
    bool4 vec =
    {
        lhs.x >= rhs,
        lhs.y >= rhs,
        lhs.z >= rhs,
        lhs.w >= rhs,
    };
    return vec;
}

pim_inline bool4 VEC_CALL f4_gteqsv(float lhs, float4 rhs)
{
    bool4 vec =
    {
        lhs >= rhs.x,
        lhs >= rhs.y,
        lhs >= rhs.z,
        lhs >= rhs.w,
    };
    return vec;
}

pim_inline float VEC_CALL f4_sum(float4 v)
{
    return v.x + v.y + v.z + v.w;
}

pim_inline float VEC_CALL f4_sum3(float4 v)
{
    return v.x + v.y + v.z;
}

pim_inline float4 VEC_CALL f4_min(float4 a, float4 b)
{
    float4 vec =
    {
        f1_min(a.x, b.x),
        f1_min(a.y, b.y),
        f1_min(a.z, b.z),
        f1_min(a.w, b.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_minvs(float4 a, float b)
{
    float4 vec =
    {
        f1_min(a.x, b),
        f1_min(a.y, b),
        f1_min(a.z, b),
        f1_min(a.w, b),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_minsv(float a, float4 b)
{
    float4 vec =
    {
        f1_min(a, b.x),
        f1_min(a, b.y),
        f1_min(a, b.z),
        f1_min(a, b.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_max(float4 a, float4 b)
{
    float4 vec =
    {
        f1_max(a.x, b.x),
        f1_max(a.y, b.y),
        f1_max(a.z, b.z),
        f1_max(a.w, b.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_maxvs(float4 a, float b)
{
    float4 vec =
    {
        f1_max(a.x, b),
        f1_max(a.y, b),
        f1_max(a.z, b),
        f1_max(a.w, b),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_maxsv(float a, float4 b)
{
    float4 vec =
    {
        f1_max(a, b.x),
        f1_max(a, b.y),
        f1_max(a, b.z),
        f1_max(a, b.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_select(float4 a, float4 b, bool4 t)
{
    float4 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
        t.z ? b.z : a.z,
        t.w ? b.w : a.w,
    };
    return c;
}

pim_inline float4 VEC_CALL f4_selectvs(float4 a, float4 b, bool t)
{
    return t ? b : a;
}

pim_inline float4 VEC_CALL f4_selectsv(float a, float b, bool4 t)
{
    float4 c =
    {
        t.x ? b : a,
        t.y ? b : a,
        t.z ? b : a,
        t.w ? b : a,
    };
    return c;
}

pim_inline float VEC_CALL f4_hmin4(float4 v)
{
    return f1_min(f1_min(v.x, v.y), f1_min(v.z, v.w));
}
pim_inline float VEC_CALL f4_hmin3(float4 v)
{
    return f1_min(v.x, f1_min(v.y, v.z));
}

pim_inline float VEC_CALL f4_hmax4(float4 v)
{
    return f1_max(f1_max(v.x, v.y), f1_max(v.z, v.w));
}
pim_inline float VEC_CALL f4_hmax3(float4 v)
{
    return f1_max(v.x, f1_max(v.y, v.z));
}

pim_inline float4 VEC_CALL f4_clamp(float4 x, float4 lo, float4 hi)
{
    return f4_min(f4_max(x, lo), hi);
}
pim_inline float4 VEC_CALL f4_clampvs(float4 x, float lo, float hi)
{
    return f4_minvs(f4_maxvs(x, lo), hi);
}
pim_inline float4 VEC_CALL f4_clampsv(float x, float4 lo, float4 hi)
{
    return f4_min(f4_maxsv(x, lo), hi);
}

pim_inline float4 VEC_CALL f4_saturate(float4 x)
{
    return f4_clampvs(x, 0.0f, 1.0f);
}

pim_inline float VEC_CALL f4_dot4(float4 a, float4 b)
{
    return f4_sum(f4_mul(a, b));
}
pim_inline float VEC_CALL f4_dot3(float4 a, float4 b)
{
    return f4_sum3(f4_mul(a, b));
}

pim_inline float VEC_CALL f4_dotsat(float4 a, float4 b)
{
    return f1_saturate(f4_dot3(a, b));
}

pim_inline float4 VEC_CALL f4_cross3(float4 a, float4 b)
{
    return f4_zxy(
        f4_sub(
            f4_mul(f4_zxy(a), b),
            f4_mul(a, f4_zxy(b))));
}

pim_inline float VEC_CALL f4_length4(float4 x)
{
    return sqrtf(f1_max(kEpsilonSq, f4_dot4(x, x)));
}
pim_inline float VEC_CALL f4_length3(float4 x)
{
    return sqrtf(f1_max(kEpsilonSq, f4_dot3(x, x)));
}

pim_inline float4 VEC_CALL f4_normalize4(float4 x)
{
    return f4_divvs(x, f4_length4(x));
}
pim_inline float4 VEC_CALL f4_normalize3(float4 x)
{
    return f4_divvs(x, f4_length3(x));
}

pim_inline float VEC_CALL f4_distance4(float4 a, float4 b)
{
    return f4_length4(f4_sub(a, b));
}
pim_inline float VEC_CALL f4_distance3(float4 a, float4 b)
{
    return f4_length3(f4_sub(a, b));
}

pim_inline float VEC_CALL f4_lengthsq4(float4 x)
{
    return f4_dot4(x, x);
}
pim_inline float VEC_CALL f4_lengthsq3(float4 x)
{
    return f4_dot3(x, x);
}

pim_inline float VEC_CALL f4_distancesq4(float4 a, float4 b)
{
    return f4_lengthsq4(f4_sub(a, b));
}
pim_inline float VEC_CALL f4_distancesq3(float4 a, float4 b)
{
    return f4_lengthsq3(f4_sub(a, b));
}

pim_inline float4 VEC_CALL f4_lerp(float4 a, float4 b, float4 t)
{
    return f4_add(a, f4_mul(f4_sub(b, a), t));
}
pim_inline float4 VEC_CALL f4_lerpvs(float4 a, float4 b, float t)
{
    return f4_add(a, f4_mulvs(f4_sub(b, a), t));
}
pim_inline float4 VEC_CALL f4_lerpsv(float a, float b, float4 t)
{
    return f4_addsv(a, f4_mulvs(t, b - a));
}

// normalizes x into [a, b] range
pim_inline float4 VEC_CALL f4_unlerp(float4 a, float4 b, float4 x)
{
    return f4_saturate(f4_div(f4_sub(x, a), f4_sub(b, a)));
}
pim_inline float4 VEC_CALL f4_unlerpvs(float4 a, float4 b, float x)
{
    return f4_saturate(f4_div(f4_subsv(x, a), f4_sub(b, a)));
}
pim_inline float4 VEC_CALL f4_unlerpsv(float a, float b, float4 x)
{
    return f4_saturate(f4_divvs(f4_subvs(x, a), b - a));
}

pim_inline float4 VEC_CALL f4_bilerp(
    float4 tl, float4 tr,
    float4 bl, float4 br,
    float2 x)
{
    return f4_lerpvs(
        f4_lerpvs(tl, tr, x.x),
        f4_lerpvs(bl, br, x.x),
        x.y);
}

pim_inline float4 VEC_CALL f4_trilerp(
    float4 tln, float4 trn,
    float4 bln, float4 brn,
    float4 tlf, float4 trf,
    float4 blf, float4 brf,
    float3 x)
{
    float4 n = f4_lerpvs(
        f4_lerpvs(tln, trn, x.x),
        f4_lerpvs(bln, brn, x.x),
        x.y);
    float4 f = f4_lerpvs(
        f4_lerpvs(tlf, trf, x.x),
        f4_lerpvs(blf, brf, x.x),
        x.y);
    return f4_lerpvs(n, f, x.z);
}

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-step
pim_inline float4 VEC_CALL f4_step(float4 a, float4 b)
{
    return f4_selectsv(0.0f, 1.0f, f4_gteq(a, b));
}

// assumes x is already within [0, 1] range
pim_inline float4 VEC_CALL f4_unormstep(float4 t)
{
    return f4_mul(f4_mul(f4_addvs(f4_mulvs(t, -2.0f), 3.0f), t), t);
}
pim_inline float4 VEC_CALL f4_unormerstep(float4 t)
{
    return f4_mul(f4_mul(f4_mul(f4_addvs(f4_mul(f4_addvs(f4_mulvs(t, 6.0f), -15.0f), t), 10.0f), t), t), t);
}

pim_inline float4 VEC_CALL f4_smoothstep(float4 a, float4 b, float4 x)
{
    return f4_unormstep(f4_unlerp(a, b, x));
}
pim_inline float4 VEC_CALL f4_smoothstepvs(float4 a, float4 b, float x)
{
    return f4_unormstep(f4_unlerpvs(a, b, x));
}
pim_inline float4 VEC_CALL f4_smoothstepsv(float a, float b, float4 x)
{
    return f4_unormstep(f4_unlerpsv(a, b, x));
}

pim_inline float4 VEC_CALL f4_smootherstep(float4 a, float4 b, float4 x)
{
    return f4_unormerstep(f4_unlerp(a, b, x));
}
pim_inline float4 VEC_CALL f4_smootherstepvs(float4 a, float4 b, float x)
{
    return f4_unormerstep(f4_unlerpvs(a, b, x));
}
pim_inline float4 VEC_CALL f4_smootherstepsv(float a, float b, float4 x)
{
    return f4_unormerstep(f4_unlerpsv(a, b, x));
}

pim_inline float4 VEC_CALL f4_reflect3(float4 i, float4 n)
{
    float4 nidn = f4_mulvs(n, f4_dot3(i, n));
    return f4_sub(i, f4_mulvs(nidn, 2.0f));
}

pim_inline float4 VEC_CALL f4_refract3(float4 i, float4 n, float k)
{
    float cosTheta = f1_min(1.0f, f4_dot3(f4_neg(i), n));
    float4 r_out_perp = f4_mulvs(f4_add(f4_mulvs(n, cosTheta), i), k);
    float4 r_out_parallel = f4_mulvs(n, -sqrtf(f1_abs(1.0f - f4_lengthsq3(r_out_perp))));
    return f4_normalize3(f4_add(r_out_perp, r_out_parallel));
}

pim_inline float4 VEC_CALL f4_sqrt(float4 v)
{
    float4 vec = { sqrtf(v.x), sqrtf(v.y), sqrtf(v.z), sqrtf(v.w) };
    return vec;
}

pim_inline float4 VEC_CALL f4_abs(float4 v)
{
    float4 vec = { f1_abs(v.x), f1_abs(v.y), f1_abs(v.z), f1_abs(v.w) };
    return vec;
}

pim_inline float4 VEC_CALL f4_pow(float4 v, float4 e)
{
    float4 vec =
    {
        f1_pow(v.x, e.x),
        f1_pow(v.y, e.y),
        f1_pow(v.z, e.z),
        f1_pow(v.w, e.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_powvs(float4 v, float e)
{
    float4 vec =
    {
        f1_pow(v.x, e),
        f1_pow(v.y, e),
        f1_pow(v.z, e),
        f1_pow(v.w, e),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_powsv(float v, float4 e)
{
    float4 vec =
    {
        f1_pow(v, e.x),
        f1_pow(v, e.y),
        f1_pow(v, e.z),
        f1_pow(v, e.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_exp(float4 v)
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

pim_inline float4 VEC_CALL f4_exp2(float4 v)
{
    float4 vec =
    {
        exp2f(v.x),
        exp2f(v.y),
        exp2f(v.z),
        exp2f(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_log(float4 v)
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

pim_inline float4 VEC_CALL f4_log2(float4 v)
{
    float4 vec =
    {
        log2f(v.x),
        log2f(v.y),
        log2f(v.z),
        log2f(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_sin(float4 v)
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

pim_inline float4 VEC_CALL f4_cos(float4 v)
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

pim_inline float4 VEC_CALL f4_tan(float4 v)
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

pim_inline float4 VEC_CALL f4_asin(float4 v)
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

pim_inline float4 VEC_CALL f4_acos(float4 v)
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

pim_inline float4 VEC_CALL f4_atan(float4 v)
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

pim_inline float4 VEC_CALL f4_trunc(float4 v)
{
    float4 vec =
    {
        f1_trunc(v.x),
        f1_trunc(v.y),
        f1_trunc(v.z),
        f1_trunc(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_floor(float4 v)
{
    float4 vec =
    {
        f1_floor(v.x),
        f1_floor(v.y),
        f1_floor(v.z),
        f1_floor(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_ceil(float4 v)
{
    float4 vec =
    {
        f1_ceil(v.x),
        f1_ceil(v.y),
        f1_ceil(v.z),
        f1_ceil(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_frac(float4 v)
{
    return f4_sub(v, f4_floor(v));
}

pim_inline float4 VEC_CALL f4_round(float4 v)
{
    float4 vec =
    {
        f1_round(v.x),
        f1_round(v.y),
        f1_round(v.z),
        f1_round(v.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_fmod(float4 a, float4 b)
{
    float4 vec =
    {
        f1_mod(a.x, b.x),
        f1_mod(a.y, b.y),
        f1_mod(a.z, b.z),
        f1_mod(a.w, b.w),
    };
    return vec;
}
pim_inline float4 VEC_CALL f4_fmodvs(float4 a, float b)
{
    float4 vec =
    {
        f1_mod(a.x, b),
        f1_mod(a.y, b),
        f1_mod(a.z, b),
        f1_mod(a.w, b),
    };
    return vec;
}
pim_inline float4 VEC_CALL f4_fmodsv(float a, float4 b)
{
    float4 vec =
    {
        f1_mod(a, b.x),
        f1_mod(a, b.y),
        f1_mod(a, b.z),
        f1_mod(a, b.w),
    };
    return vec;
}

pim_inline float4 VEC_CALL f4_wrap(float4 x)
{
    x.x = f1_wrap(x.x);
    x.y = f1_wrap(x.y);
    x.z = f1_wrap(x.z);
    x.w = f1_wrap(x.w);
    return x;
}

pim_inline float4 VEC_CALL f4_rad(float4 x)
{
    return f4_mulvs(x, kRadiansPerDegree);
}

pim_inline float4 VEC_CALL f4_deg(float4 x)
{
    return f4_mulvs(x, kDegreesPerRadian);
}

pim_inline float4 VEC_CALL f4_blend(float4 a, float4 b, float4 c, float4 wuvt)
{
    float4 p = f4_mulvs(a, wuvt.x);
    p = f4_add(p, f4_mulvs(b, wuvt.y));
    p = f4_add(p, f4_mulvs(c, wuvt.z));
    return p;
}

pim_inline float4 VEC_CALL f4_unorm(float4 s)
{
    return f4_addvs(f4_mulvs(s, 0.5f), 0.5f);
}

pim_inline float4 VEC_CALL f4_snorm(float4 u)
{
    return f4_subvs(f4_mulvs(u, 2.0f), 1.0f);
}

pim_inline float4 VEC_CALL f4_rand(Prng *const pim_noalias rng)
{
    return Prng_float4(rng);
}

PIM_C_END
