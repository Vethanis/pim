#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"

static const int3 i3_0 = { 0, 0, 0 };
static const int3 i3_1 = { 1, 1, 1 };
static const int3 i3_2 = { 2, 2, 2 };

pim_inline int3 VEC_CALL i3_v(i32 x, i32 y, i32 z)
{
    int3 vec = { x, y, z };
    return vec;
}

pim_inline int3 VEC_CALL i3_s(i32 s)
{
    int3 vec = { s, s, s };
    return vec;
}

pim_inline int3 VEC_CALL i3_fv(float x, float y, float z)
{
    int3 vec = { (i32)x, (i32)y, (i32)z };
    return vec;
}

pim_inline int3 VEC_CALL i3_fs(float s)
{
    int3 vec = { (i32)s, (i32)s, (i32)s };
    return vec;
}

pim_inline float3 VEC_CALL i3_f3(int3 v)
{
    float3 f = { (float)v.x, (float)v.y, (float)v.z };
    return f;
}

pim_inline int3 VEC_CALL i3_xx(int3 v)
{
    int3 vec = { v.x, v.x, v.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_yy(int3 v)
{
    int3 vec = { v.y, v.y, v.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_yx(int3 v)
{
    int3 vec = { v.y, v.x, v.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_load(const i32* src)
{
    int3 vec = { src[0], src[1], src[2] };
    return vec;
}

pim_inline void VEC_CALL i3_store(int3 src, i32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
}

pim_inline int3 VEC_CALL i3_add(int3 lhs, int3 rhs)
{
    int3 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_sub(int3 lhs, int3 rhs)
{
    int3 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_mul(int3 lhs, int3 rhs)
{
    int3 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_div(int3 lhs, int3 rhs)
{
    int3 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z };
    return vec;
}

pim_inline int3 VEC_CALL i3_neg(int3 v)
{
    int3 vec = { -v.x, -v.y, -v.z};
    return vec;
}

pim_inline int3 VEC_CALL i3_eq(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
        lhs.z == rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_neq(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
        lhs.z != rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_lt(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
        lhs.z < rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_ltvs(int3 lhs, i32 rhs)
{
    int3 vec =
    {
        lhs.x < rhs,
        lhs.y < rhs,
        lhs.z < rhs,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_ltsv(i32 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs < rhs.x,
        lhs < rhs.y,
        lhs < rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_gt(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
        lhs.z > rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_gtvs(int3 lhs, i32 rhs)
{
    int3 vec =
    {
        lhs.x > rhs,
        lhs.y > rhs,
        lhs.z > rhs,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_gtsv(i32 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs > rhs.x,
        lhs > rhs.y,
        lhs > rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_lteq(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
        lhs.z <= rhs.z,
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_gteq(int3 lhs, int3 rhs)
{
    int3 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
        lhs.z >= rhs.z,
    };
    return vec;
}

pim_inline i32 VEC_CALL i3_sum(int3 v)
{
    return v.x + v.y + v.z;
}

pim_inline bool VEC_CALL i3_any(int3 b)
{
    return i3_sum(b) != 0;
}

pim_inline bool VEC_CALL i3_all(int3 b)
{
    return i3_sum(b) == 3;
}

pim_inline int3 VEC_CALL i3_not(int3 b)
{
    return i3_sub(i3_1, b);
}

pim_inline int3 VEC_CALL i3_abs(int3 v)
{
    int3 vec = { v.x &= 0x7fffffff, v.y &= 0x7fffffff, v.z &= 0x7fffffff };
    return vec;
}

pim_inline int3 VEC_CALL i3_min(int3 a, int3 b)
{
    int3 vec =
    {
        i1_min(a.x, b.x),
        i1_min(a.y, b.y),
        i1_min(a.z, b.z),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_minvs(int3 a, i32 b)
{
    int3 vec =
    {
        i1_min(a.x, b),
        i1_min(a.y, b),
        i1_min(a.z, b),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_minsv(i32 a, int3 b)
{
    int3 vec =
    {
        i1_min(a, b.x),
        i1_min(a, b.y),
        i1_min(a, b.z),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_max(int3 a, int3 b)
{
    int3 vec =
    {
        i1_max(a.x, b.x),
        i1_max(a.y, b.y),
        i1_max(a.z, b.z),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_maxvs(int3 a, i32 b)
{
    int3 vec =
    {
        i1_max(a.x, b),
        i1_max(a.y, b),
        i1_max(a.z, b),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_maxsv(i32 a, int3 b)
{
    int3 vec =
    {
        i1_max(a, b.x),
        i1_max(a, b.y),
        i1_max(a, b.z),
    };
    return vec;
}

pim_inline int3 VEC_CALL i3_select(int3 a, int3 b, int3 t)
{
    int3 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
        t.z ? b.z : a.z,
    };
    return c;
}

pim_inline i32 VEC_CALL i3_hmin(int3 v)
{
    return i1_min(i1_min(v.x, v.y), v.z);
}

pim_inline i32 VEC_CALL i3_hmax(int3 v)
{
    return i1_max(i1_max(v.x, v.y), v.z);
}

pim_inline int3 VEC_CALL i3_clamp(int3 x, int3 lo, int3 hi)
{
    return i3_min(i3_max(x, lo), hi);
}

pim_inline int3 VEC_CALL i3_clampvs(int3 x, i32 lo, i32 hi)
{
    return i3_minvs(i3_maxvs(x, lo), hi);
}

pim_inline i32 VEC_CALL i3_dot(int3 a, int3 b)
{
    return i3_sum(i3_mul(a, b));
}

pim_inline i32 VEC_CALL i3_length(int3 x)
{
    return (i32)sqrt((double)i3_dot(x, x));
}

pim_inline i32 VEC_CALL i3_distance(int3 a, int3 b)
{
    return i3_length(i3_sub(a, b));
}

pim_inline i32 VEC_CALL i3_lengthsq(int3 x)
{
    return i3_dot(x, x);
}

pim_inline i32 VEC_CALL i3_distancesq(int3 a, int3 b)
{
    return i3_lengthsq(i3_sub(a, b));
}

pim_inline int3 VEC_CALL i3_lerp(int3 a, int3 b, i32 t)
{
    int3 vt = i3_s(t);
    int3 ba = i3_sub(b, a);
    b = i3_mul(ba, vt);
    return i3_add(a, b);
}

PIM_C_END
