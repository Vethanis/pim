#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

#define i4_0 i4_s(0)
#define i4_1 i4_s(1)
#define i4_2 i4_s(2)

pim_inline int4 VEC_CALL i4_v(i32 x, i32 y, i32 z, i32 w)
{
    int4 vec = { x, y, z, w };
    return vec;
}

pim_inline int4 VEC_CALL i4_s(i32 s)
{
    int4 vec = { s, s, s, s };
    return vec;
}

pim_inline int4 VEC_CALL i4_fv(float x, float y, float z, float w)
{
    int4 vec = { (i32)x, (i32)y, (i32)z, (i32)w };
    return vec;
}

pim_inline int4 VEC_CALL i4_fs(float s)
{
    i32 is = (i32)s;
    int4 vec = { is, is, is, is };
    return vec;
}

pim_inline float4 VEC_CALL i4_f4(int4 v)
{
    float4 f = { (float)v.x, (float)v.y, (float)v.z, (float)v.w };
    return f;
}

pim_inline int4 VEC_CALL i4_xx(int4 v)
{
    int4 vec = { v.x, v.x, v.z, v.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_yy(int4 v)
{
    int4 vec = { v.y, v.y, v.z, v.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_yx(int4 v)
{
    int4 vec = { v.y, v.x, v.z, v.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_load(const i32* src)
{
    int4 vec = { src[0], src[1], src[2], src[3] };
    return vec;
}

pim_inline void VEC_CALL i4_store(int4 src, i32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
    dst[3] = src.w;
}

pim_inline int4 VEC_CALL i4_add(int4 lhs, int4 rhs)
{
    int4 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return vec;
}
pim_inline int4 VEC_CALL i4_addvs(int4 lhs, i32 rhs)
{
    int4 vec = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs };
    return vec;
}
pim_inline int4 VEC_CALL i4_addsv(i32 lhs, int4 rhs)
{
    int4 vec = { lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_sub(int4 lhs, int4 rhs)
{
    int4 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return vec;
}
pim_inline int4 VEC_CALL i4_subvs(int4 lhs, i32 rhs)
{
    int4 vec = { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs };
    return vec;
}
pim_inline int4 VEC_CALL i4_subsv(i32 lhs, int4 rhs)
{
    int4 vec = { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_mul(int4 lhs, int4 rhs)
{
    int4 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
    return vec;
}
pim_inline int4 VEC_CALL i4_mulvs(int4 lhs, i32 rhs)
{
    int4 vec = { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
    return vec;
}
pim_inline int4 VEC_CALL i4_mulsv(i32 lhs, int4 rhs)
{
    int4 vec = { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_div(int4 lhs, int4 rhs)
{
    int4 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
    return vec;
}
pim_inline int4 VEC_CALL i4_divvs(int4 lhs, i32 rhs)
{
    int4 vec = { lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs };
    return vec;
}
pim_inline int4 VEC_CALL i4_divsv(i32 lhs, int4 rhs)
{
    int4 vec = { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w };
    return vec;
}

pim_inline int4 VEC_CALL i4_neg(int4 v)
{
    int4 vec = { -v.x, -v.y, -v.z, -v.w };
    return vec;
}

pim_inline bool4 VEC_CALL i4_eq(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_eqvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_eqsv(i32 lhs, int4 rhs)
{
    bool4 vec =
    {
        lhs == rhs.x,
        lhs == rhs.y,
        lhs == rhs.z,
        lhs == rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL i4_neq(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_neqvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_neqsv(i32 lhs, int4 rhs)
{
    bool4 vec =
    {
        lhs != rhs.x,
        lhs != rhs.y,
        lhs != rhs.z,
        lhs != rhs.w,
    };
    return vec;
}

pim_inline bool4 VEC_CALL i4_lt(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_ltvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_ltsv(i32 lhs, int4 rhs)
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

pim_inline bool4 VEC_CALL i4_gt(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_gtvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_gtsv(i32 lhs, int4 rhs)
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

pim_inline bool4 VEC_CALL i4_lteq(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_lteqvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_lteqsv(i32 lhs, int4 rhs)
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

pim_inline bool4 VEC_CALL i4_gteq(int4 lhs, int4 rhs)
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
pim_inline bool4 VEC_CALL i4_gteqvs(int4 lhs, i32 rhs)
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
pim_inline bool4 VEC_CALL i4_gteqsv(i32 lhs, int4 rhs)
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

pim_inline i32 VEC_CALL i4_sum4(int4 v)
{
    return v.x + v.y + v.z + v.w;
}
pim_inline i32 VEC_CALL i4_sum3(int4 v)
{
    return v.x + v.y + v.z;
}

pim_inline int4 VEC_CALL i4_abs(int4 v)
{
    int4 vec = { i1_abs(v.x), i1_abs(v.y), i1_abs(v.z), i1_abs(v.w) };
    return vec;
}

pim_inline int4 VEC_CALL i4_min(int4 a, int4 b)
{
    int4 vec =
    {
        i1_min(a.x, b.x),
        i1_min(a.y, b.y),
        i1_min(a.z, b.z),
        i1_min(a.w, b.w),
    };
    return vec;
}
pim_inline int4 VEC_CALL i4_minvs(int4 a, i32 b)
{
    int4 vec =
    {
        i1_min(a.x, b),
        i1_min(a.y, b),
        i1_min(a.z, b),
        i1_min(a.w, b),
    };
    return vec;
}
pim_inline int4 VEC_CALL i4_minsv(i32 a, int4 b)
{
    int4 vec =
    {
        i1_min(a, b.x),
        i1_min(a, b.y),
        i1_min(a, b.z),
        i1_min(a, b.w),
    };
    return vec;
}

pim_inline int4 VEC_CALL i4_max(int4 a, int4 b)
{
    int4 vec =
    {
        i1_max(a.x, b.x),
        i1_max(a.y, b.y),
        i1_max(a.z, b.z),
        i1_max(a.z, b.w),
    };
    return vec;
}
pim_inline int4 VEC_CALL i4_maxvs(int4 a, i32 b)
{
    int4 vec =
    {
        i1_max(a.x, b),
        i1_max(a.y, b),
        i1_max(a.z, b),
        i1_max(a.w, b),
    };
    return vec;
}
pim_inline int4 VEC_CALL i4_maxsv(i32 a, int4 b)
{
    int4 vec =
    {
        i1_max(a, b.x),
        i1_max(a, b.y),
        i1_max(a, b.z),
        i1_max(a, b.w),
    };
    return vec;
}

pim_inline int4 VEC_CALL i4_select(int4 a, int4 b, int4 t)
{
    int4 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
        t.z ? b.z : a.z,
        t.w ? b.w : a.w,
    };
    return c;
}
pim_inline int4 VEC_CALL i4_selectvs(int4 a, int4 b, i32 t)
{
    int4 c =
    {
        t ? b.x : a.x,
        t ? b.y : a.y,
        t ? b.z : a.z,
        t ? b.w : a.w,
    };
    return c;
}
pim_inline int4 VEC_CALL i4_selectsv(i32 a, i32 b, int4 t)
{
    int4 c =
    {
        t.x ? b : a,
        t.y ? b : a,
        t.z ? b : a,
        t.w ? b : a,
    };
    return c;
}

pim_inline i32 VEC_CALL i4_hmin(int4 v)
{
    return i1_min(
        i1_min(v.x, v.y),
        i1_min(v.z, v.w));
}

pim_inline i32 VEC_CALL i4_hmax(int4 v)
{
    return i1_max(
        i1_max(v.x, v.y),
        i1_max(v.z, v.w));
}

pim_inline int4 VEC_CALL i4_clamp(int4 x, int4 lo, int4 hi)
{
    return i4_min(i4_max(x, lo), hi);
}
pim_inline int4 VEC_CALL i4_clampvs(int4 x, i32 lo, i32 hi)
{
    return i4_minvs(i4_maxvs(x, lo), hi);
}
pim_inline int4 VEC_CALL i4_clampsv(i32 x, int4 lo, int4 hi)
{
    return i4_min(i4_maxsv(x, lo), hi);
}

pim_inline i32 VEC_CALL i4_dot4(int4 a, int4 b)
{
    return i4_sum4(i4_mul(a, b));
}
pim_inline i32 VEC_CALL i4_dot3(int4 a, int4 b)
{
    return i4_sum3(i4_mul(a, b));
}

pim_inline i32 VEC_CALL i4_lengthsq4(int4 x)
{
    return i4_dot4(x, x);
}
pim_inline i32 VEC_CALL i4_lengthsq3(int4 x)
{
    return i4_dot3(x, x);
}

pim_inline i32 VEC_CALL i4_length4(int4 x)
{
    return (i32)(sqrt((double)i4_dot4(x, x)) + 0.5);
}
pim_inline i32 VEC_CALL i4_length3(int4 x)
{
    return (i32)(sqrt((double)i4_dot3(x, x)) + 0.5);
}

pim_inline i32 VEC_CALL i4_distancesq4(int4 a, int4 b)
{
    return i4_lengthsq4(i4_sub(a, b));
}
pim_inline i32 VEC_CALL i4_distancesq3(int4 a, int4 b)
{
    return i4_lengthsq3(i4_sub(a, b));
}

pim_inline i32 VEC_CALL i4_distance4(int4 a, int4 b)
{
    return i4_length4(i4_sub(a, b));
}
pim_inline i32 VEC_CALL i4_distance3(int4 a, int4 b)
{
    return i4_length3(i4_sub(a, b));
}

pim_inline int4 VEC_CALL i4_lerp(int4 a, int4 b, int4 t)
{
    return i4_add(a, i4_mul(i4_sub(b, a), t));
}
pim_inline int4 VEC_CALL i4_lerpvs(int4 a, int4 b, i32 t)
{
    return i4_add(a, i4_mulvs(i4_sub(b, a), t));
}
pim_inline int4 VEC_CALL i4_lerpsv(i32 a, i32 b, int4 t)
{
    return i4_addsv(a, i4_mulsv(b - a, t));
}

PIM_C_END
