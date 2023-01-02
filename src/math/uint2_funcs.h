#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

#define u2_0 u2_s(0)
#define u2_1 u2_s(1)
#define u2_2 u2_s(2)

pim_inline uint2 VEC_CALL u2_v(u32 x, u32 y)
{
    uint2 vec = { x, y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_s(u32 s)
{
    uint2 vec = { s, s };
    return vec;
}

pim_inline uint2 VEC_CALL u2_fv(float x, float y)
{
    uint2 vec = { (u32)x, (u32)y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_fs(float s)
{
    uint2 vec = { (u32)s, (u32)s };
    return vec;
}

pim_inline float2 VEC_CALL u2_f2(uint2 v)
{
    float2 f = { (float)v.x, (float)v.y };
    return f;
}

pim_inline uint2 VEC_CALL u2_xx(uint2 v)
{
    uint2 vec = { v.x, v.x };
    return vec;
}

pim_inline uint2 VEC_CALL u2_yy(uint2 v)
{
    uint2 vec = { v.y, v.y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_yx(uint2 v)
{
    uint2 vec = { v.y, v.x };
    return vec;
}

pim_inline uint2 VEC_CALL u2_load(const u32* src)
{
    uint2 vec = { src[0], src[1] };
    return vec;
}

pim_inline void VEC_CALL u2_store(uint2 src, u32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
}

pim_inline uint2 VEC_CALL u2_add(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x + rhs.x, lhs.y + rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_addvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x + rhs, lhs.y + rhs };
    return vec;
}
pim_inline uint2 VEC_CALL u2_addsv(u32 lhs, uint2 rhs)
{
    uint2 vec = { lhs + rhs.x, lhs + rhs.y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_sub(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x - rhs.x, lhs.y - rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_subvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x - rhs, lhs.y - rhs };
    return vec;
}
pim_inline uint2 VEC_CALL u2_subsv(u32 lhs, uint2 rhs)
{
    uint2 vec = { lhs - rhs.x, lhs - rhs.y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_mul(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x * rhs.x, lhs.y * rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_mulvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x * rhs, lhs.y * rhs };
    return vec;
}
pim_inline uint2 VEC_CALL u2_mulsv(u32 lhs, uint2 rhs)
{
    uint2 vec = { lhs * rhs.x, lhs * rhs.y };
    return vec;
}

pim_inline uint2 VEC_CALL u2_div(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x / rhs.x, lhs.y / rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_divvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x / rhs, lhs.y / rhs };
    return vec;
}
pim_inline uint2 VEC_CALL u2_divsv(u32 lhs, uint2 rhs)
{
    uint2 vec = { lhs / rhs.x, lhs / rhs.y };
    return vec;
}

// 'remainder' operator; can yield negative values
pim_inline uint2 VEC_CALL u2_rem(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x % rhs.x, lhs.y % rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_remvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x % rhs, lhs.y % rhs };
    return vec;
}
pim_inline uint2 VEC_CALL u2_remsv(u32 lhs, uint2 rhs)
{
    uint2 vec = { lhs % rhs.x, lhs % rhs.y };
    return vec;
}

// euclidean modulus; always positive
pim_inline uint2 VEC_CALL u2_mod(uint2 lhs, uint2 rhs)
{
    uint2 vec = u2_rem(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs.x : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs.y : vec.y;
    return vec;
}
pim_inline uint2 VEC_CALL u2_modvs(uint2 lhs, u32 rhs)
{
    uint2 vec = u2_remvs(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs : vec.y;
    return vec;
}
pim_inline uint2 VEC_CALL u2_modsv(u32 lhs, uint2 rhs)
{
    uint2 vec = u2_remsv(lhs, rhs);
    vec.x = vec.x < 0 ? vec.x + rhs.x : vec.x;
    vec.y = vec.y < 0 ? vec.y + rhs.y : vec.y;
    return vec;
}

pim_inline uint2 VEC_CALL u2_neg(uint2 v)
{
    uint2 vec = { -v.x, -v.y };
    return vec;
}

pim_inline bool2 VEC_CALL u2_eq(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_eqvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x == rhs,
        lhs.y == rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_eqsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs == rhs.x,
        lhs == rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL u2_neq(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_neqvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x != rhs,
        lhs.y != rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_neqsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs != rhs.x,
        lhs != rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL u2_lt(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_ltvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x < rhs,
        lhs.y < rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_ltsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs < rhs.x,
        lhs < rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL u2_gt(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_gtvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x > rhs,
        lhs.y > rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_gtsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs > rhs.x,
        lhs > rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL u2_lteq(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_lteqvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x <= rhs,
        lhs.y <= rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_lteqsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs <= rhs.x,
        lhs <= rhs.y,
    };
    return vec;
}

pim_inline bool2 VEC_CALL u2_gteq(uint2 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_gteqvs(uint2 lhs, u32 rhs)
{
    bool2 vec =
    {
        lhs.x >= rhs,
        lhs.y >= rhs,
    };
    return vec;
}
pim_inline bool2 VEC_CALL u2_gteqsv(u32 lhs, uint2 rhs)
{
    bool2 vec =
    {
        lhs >= rhs.x,
        lhs >= rhs.y,
    };
    return vec;
}

pim_inline u32 VEC_CALL u2_sum(uint2 v)
{
    return v.x + v.y;
}

pim_inline uint2 VEC_CALL u2_and(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x & rhs.x, lhs.y & rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_andvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x & rhs, lhs.y & rhs };
    return vec;
}

pim_inline uint2 VEC_CALL u2_or(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x | rhs.x, lhs.y | rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_orvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x | rhs, lhs.y | rhs };
    return vec;
}

pim_inline uint2 VEC_CALL u2_xor(uint2 lhs, uint2 rhs)
{
    uint2 vec = { lhs.x ^ rhs.x, lhs.y ^ rhs.y };
    return vec;
}
pim_inline uint2 VEC_CALL u2_xorvs(uint2 lhs, u32 rhs)
{
    uint2 vec = { lhs.x ^ rhs, lhs.y ^ rhs };
    return vec;
}

pim_inline uint2 VEC_CALL u2_min(uint2 a, uint2 b)
{
    uint2 vec =
    {
        u1_min(a.x, b.x),
        u1_min(a.y, b.y),
    };
    return vec;
}
pim_inline uint2 VEC_CALL u2_minvs(uint2 a, u32 b)
{
    uint2 vec =
    {
        u1_min(a.x, b),
        u1_min(a.y, b),
    };
    return vec;
}
pim_inline uint2 VEC_CALL u2_minsv(u32 a, uint2 b)
{
    uint2 vec =
    {
        u1_min(a, b.x),
        u1_min(a, b.y),
    };
    return vec;
}

pim_inline uint2 VEC_CALL u2_max(uint2 a, uint2 b)
{
    uint2 vec =
    {
        u1_max(a.x, b.x),
        u1_max(a.y, b.y),
    };
    return vec;
}
pim_inline uint2 VEC_CALL u2_maxvs(uint2 a, u32 b)
{
    uint2 vec =
    {
        u1_max(a.x, b),
        u1_max(a.y, b),
    };
    return vec;
}
pim_inline uint2 VEC_CALL u2_maxsv(u32 a, uint2 b)
{
    uint2 vec =
    {
        u1_max(a, b.x),
        u1_max(a, b.y),
    };
    return vec;
}

pim_inline uint2 VEC_CALL u2_select(uint2 a, uint2 b, uint2 t)
{
    uint2 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
    };
    return c;
}
pim_inline uint2 VEC_CALL u2_selectvs(uint2 a, uint2 b, u32 t)
{
    uint2 c =
    {
        t ? b.x : a.x,
        t ? b.y : a.y,
    };
    return c;
}
pim_inline uint2 VEC_CALL u2_selectsv(u32 a, u32 b, uint2 t)
{
    uint2 c =
    {
        t.x ? b : a,
        t.y ? b : a,
    };
    return c;
}

pim_inline u32 VEC_CALL u2_hmin(uint2 v)
{
    return u1_min(v.x, v.y);
}

pim_inline u32 VEC_CALL u2_hmax(uint2 v)
{
    return u1_max(v.x, v.y);
}

pim_inline uint2 VEC_CALL u2_clamp(uint2 x, uint2 lo, uint2 hi)
{
    return u2_min(u2_max(x, lo), hi);
}
pim_inline uint2 VEC_CALL u2_clampvs(uint2 x, u32 lo, u32 hi)
{
    return u2_minvs(u2_maxvs(x, lo), hi);
}
pim_inline uint2 VEC_CALL u2_clampsv(u32 x, uint2 lo, uint2 hi)
{
    return u2_min(u2_maxsv(x, lo), hi);
}

pim_inline u32 VEC_CALL u2_dot(uint2 a, uint2 b)
{
    return u2_sum(u2_mul(a, b));
}
pim_inline u32 VEC_CALL u2_dotvs(uint2 a, u32 b)
{
    return u2_sum(u2_mulvs(a, b));
}
pim_inline u32 VEC_CALL u2_dotsv(u32 a, uint2 b)
{
    return u2_sum(u2_mulsv(a, b));
}

pim_inline u32 VEC_CALL u2_length(uint2 x)
{
    return (u32)(sqrt((double)u2_dot(x, x)) + 0.5);
}

pim_inline u32 VEC_CALL u2_distance(uint2 a, uint2 b)
{
    return u2_length(u2_sub(a, b));
}
pim_inline u32 VEC_CALL u2_distancevs(uint2 a, u32 b)
{
    return u2_length(u2_subvs(a, b));
}
pim_inline u32 VEC_CALL u2_distancesv(u32 a, uint2 b)
{
    return u2_length(u2_subsv(a, b));
}

pim_inline u32 VEC_CALL u2_lengthsq(uint2 x)
{
    return u2_dot(x, x);
}

pim_inline u32 VEC_CALL u2_distancesq(uint2 a, uint2 b)
{
    return u2_lengthsq(u2_sub(a, b));
}

pim_inline uint2 VEC_CALL u2_lerp(uint2 a, uint2 b, uint2 t)
{
    return u2_add(a, u2_mul(u2_sub(b, a), t));
}
pim_inline uint2 VEC_CALL u2_lerpvs(uint2 a, uint2 b, u32 t)
{
    return u2_add(a, u2_mulvs(u2_sub(b, a), t));
}
pim_inline uint2 VEC_CALL u2_lerpsv(u32 a, u32 b, uint2 t)
{
    return u2_addsv(a, u2_mulsv(b - a, t));
}

PIM_C_END
