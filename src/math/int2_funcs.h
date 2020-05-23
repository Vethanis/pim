#pragma once

#include "math/types.h"

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

// 'remainder' operator; can yield negative values
pim_inline int2 VEC_CALL i2_rem(int2 lhs, int2 rhs)
{
    int2 vec = { lhs.x % rhs.x, lhs.y % rhs.y };
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

pim_inline int2 VEC_CALL i2_neg(int2 v)
{
    int2 vec = { -v.x, -v.y };
    return vec;
}

pim_inline int2 VEC_CALL i2_eq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_neq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_lt(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_gt(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_lteq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
    };
    return vec;
}

pim_inline int2 VEC_CALL i2_gteq(int2 lhs, int2 rhs)
{
    int2 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
    };
    return vec;
}

pim_inline i32 VEC_CALL i2_sum(int2 v)
{
    return v.x + v.y;
}

pim_inline bool VEC_CALL i2_any(int2 b)
{
    return i2_sum(b) != 0;
}

pim_inline bool VEC_CALL i2_all(int2 b)
{
    return i2_sum(b) == 2;
}

pim_inline int2 VEC_CALL i2_not(int2 b)
{
    return i2_sub(i2_1, b);
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

pim_inline i32 VEC_CALL i2_dot(int2 a, int2 b)
{
    return i2_sum(i2_mul(a, b));
}

pim_inline i32 VEC_CALL i2_length(int2 x)
{
    return (i32)sqrt((double)i2_dot(x, x));
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

PIM_C_END
