#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

#define i2_0 i2_s(0)
#define i2_1 i2_s(1)
#define i2_2 i2_s(2)

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

pim_inline float2 VEC_CALL i2_f2(int2 v)
{
    float2 f = { (float)v.x, (float)v.y };
    return f;
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
pim_inline int2 VEC_CALL i2_addvs(int2 lhs, i32 rhs)
{
    int2 vec = { lhs.x + rhs, lhs.y + rhs };
    return vec;
}
pim_inline int2 VEC_CALL i2_addsv(i32 lhs, int2 rhs)
{
    int2 vec = { lhs + rhs.x, lhs + rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_sub(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x - rhs.x, lhs.y - rhs.y };
    return vec;
}
pim_inline int2 VEC_CALL i2_subvs(int2 lhs, i32 rhs)
{
    int2 vec = { lhs.x - rhs, lhs.y - rhs };
    return vec;
}
pim_inline int2 VEC_CALL i2_subsv(i32 lhs, int2 rhs)
{
    int2 vec = { lhs - rhs.x, lhs - rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_mul(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x * rhs.x, lhs.y * rhs.y };
    return vec;
}
pim_inline int2 VEC_CALL i2_mulvs(int2 lhs, i32 rhs)
{
    int2 vec = { lhs.x * rhs, lhs.y * rhs };
    return vec;
}
pim_inline int2 VEC_CALL i2_mulsv(i32 lhs, int2 rhs)
{
    int2 vec = { lhs * rhs.x, lhs * rhs.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_div(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x / rhs.x, lhs.y / rhs.y };
    return vec;
}
pim_inline int2 VEC_CALL i2_divvs(int2 lhs, i32 rhs)
{
    int2 vec = { lhs.x / rhs, lhs.y / rhs };
    return vec;
}
pim_inline int2 VEC_CALL i2_divsv(i32 lhs, int2 rhs)
{
    int2 vec = { lhs / rhs.x, lhs / rhs.y };
    return vec;
}

// 'remainder' operator; can yield negative values
pim_inline int2 VEC_CALL i2_rem(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x % rhs.x, lhs.y % rhs.y };
    return vec;
}
pim_inline int2 VEC_CALL i2_remvs(int2 lhs, i32 rhs)
{
    int2 vec = { lhs.x % rhs, lhs.y % rhs };
    return vec;
}
pim_inline int2 VEC_CALL i2_remsv(i32 lhs, int2 rhs)
{
    int2 vec = { lhs % rhs.x, lhs % rhs.y };
    return vec;
}

// euclidean modulus; always positive
pim_inline int2 VEC_CALL i2_mod(int2 lhs, int2 rhs)
{
    int2 vec = i2_rem(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs.x : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs.y : vec.y;
    return vec;
}
pim_inline int2 VEC_CALL i2_modvs(int2 lhs, i32 rhs)
{
    int2 vec = i2_remvs(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs : vec.y;
    return vec;
}
pim_inline int2 VEC_CALL i2_modsv(i32 lhs, int2 rhs)
{
    int2 vec = i2_remsv(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs.x : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs.y : vec.y;
    return vec;
}

pim_inline int2 VEC_CALL i2_neg(int2 v)
{
    int2 vec = { -v.x, -v.y };
    return vec;
}

pim_inline bool2 VEC_CALL i2_eq(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_eqvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x == rhs,
        lhs.y == rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_eqsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs == rhs.x,
        lhs == rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL i2_neq(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_neqvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x != rhs,
        lhs.y != rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_neqsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs != rhs.x,
        lhs != rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL i2_lt(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_ltvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x < rhs,
        lhs.y < rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_ltsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs < rhs.x,
        lhs < rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL i2_gt(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_gtvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x > rhs,
        lhs.y > rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_gtsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs > rhs.x,
        lhs > rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL i2_lteq(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_lteqvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x <= rhs,
        lhs.y <= rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_lteqsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs <= rhs.x,
        lhs <= rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL i2_gteq(int2 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_gteqvs(int2 lhs, i32 rhs)
{
    bool2 vec =
    {
        lhs.x >= rhs,
        lhs.y >= rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL i2_gteqsv(i32 lhs, int2 rhs)
{
    bool2 vec =
    {
        lhs >= rhs.x,
        lhs >= rhs.y,
    };
    return vec;
}

pim_inline i32 VEC_CALL i2_sum(int2 v)
{
    return v.x + v.y;
}

pim_inline int2 VEC_CALL i2_abs(int2 v)
{
    int2 vec = { i1_abs(v.x), i1_abs(v.y) };
    return vec;
}

pim_inline int2 VEC_CALL i2_min(int2 a, int2 b)
{
    int2 vec =
    {
        i1_min(a.x, b.x),
        i1_min(a.y, b.y),
    };
    return vec;
}
pim_inline int2 VEC_CALL i2_minvs(int2 a, i32 b)
{
    int2 vec =
    {
        i1_min(a.x, b),
        i1_min(a.y, b),
    };
    return vec;
}
pim_inline int2 VEC_CALL i2_minsv(i32 a, int2 b)
{
    int2 vec =
    {
        i1_min(a, b.x),
        i1_min(a, b.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_max(int2 a, int2 b)
{
    int2 vec =
    {
        i1_max(a.x, b.x),
        i1_max(a.y, b.y),
    };
    return vec;
}
pim_inline int2 VEC_CALL i2_maxvs(int2 a, i32 b)
{
    int2 vec =
    {
        i1_max(a.x, b),
        i1_max(a.y, b),
    };
    return vec;
}
pim_inline int2 VEC_CALL i2_maxsv(i32 a, int2 b)
{
    int2 vec =
    {
        i1_max(a, b.x),
        i1_max(a, b.y),
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_select(int2 a, int2 b, int2 t)
{
    int2 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
    };
    return c;
}
pim_inline int2 VEC_CALL i2_selectvs(int2 a, int2 b, i32 t)
{
    int2 c =
    {
        t ? b.x : a.x,
        t ? b.y : a.y,
    };
    return c;
}
pim_inline int2 VEC_CALL i2_selectsv(i32 a, i32 b, int2 t)
{
    int2 c =
    {
        t.x ? b : a,
        t.y ? b : a,
    };
    return c;
}

pim_inline i32 VEC_CALL i2_hmin(int2 v)
{
    return i1_min(v.x, v.y);
}

pim_inline i32 VEC_CALL i2_hmax(int2 v)
{
    return i1_max(v.x, v.y);
}

pim_inline int2 VEC_CALL i2_clamp(int2 x, int2 lo, int2 hi)
{
    return i2_min(i2_max(x, lo), hi);
}
pim_inline int2 VEC_CALL i2_clampvs(int2 x, i32 lo, i32 hi)
{
    return i2_minvs(i2_maxvs(x, lo), hi);
}
pim_inline int2 VEC_CALL i2_clampsv(i32 x, int2 lo, int2 hi)
{
    return i2_min(i2_maxsv(x, lo), hi);
}

pim_inline i32 VEC_CALL i2_dot(int2 a, int2 b)
{
    return i2_sum(i2_mul(a, b));
}
pim_inline i32 VEC_CALL i2_dotvs(int2 a, i32 b)
{
    return i2_sum(i2_mulvs(a, b));
}
pim_inline i32 VEC_CALL i2_dotsv(i32 a, int2 b)
{
    return i2_sum(i2_mulsv(a, b));
}

pim_inline i32 VEC_CALL i2_length(int2 x)
{
    return (i32)(sqrt((double)i2_dot(x, x)) + 0.5);
}

pim_inline i32 VEC_CALL i2_distance(int2 a, int2 b)
{
    return i2_length(i2_sub(a, b));
}
pim_inline i32 VEC_CALL i2_distancevs(int2 a, i32 b)
{
    return i2_length(i2_subvs(a, b));
}
pim_inline i32 VEC_CALL i2_distancesv(i32 a, int2 b)
{
    return i2_length(i2_subsv(a, b));
}

pim_inline i32 VEC_CALL i2_lengthsq(int2 x)
{
    return i2_dot(x, x);
}

pim_inline i32 VEC_CALL i2_distancesq(int2 a, int2 b)
{
    return i2_lengthsq(i2_sub(a, b));
}

pim_inline int2 VEC_CALL i2_lerp(int2 a, int2 b, int2 t)
{
    return i2_add(a, i2_mul(i2_sub(b, a), t));
}
pim_inline int2 VEC_CALL i2_lerpvs(int2 a, int2 b, i32 t)
{
    return i2_add(a, i2_mulvs(i2_sub(b, a), t));
}
pim_inline int2 VEC_CALL i2_lerpsv(i32 a, i32 b, int2 t)
{
    return i2_addsv(a, i2_mulsv(b - a, t));
}

PIM_C_END
