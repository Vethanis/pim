#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

#define u4_0 u4_s(0)
#define u4_1 u4_s(1)
#define u4_2 u4_s(2)

pim_inline uint4 VEC_CALL u4_v(u32 x, u32 y, u32 z, u32 w)
{
    uint4 vec = { x, y, z, w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_s(u32 s)
{
    uint4 vec = { s, s, s, s };
    return vec;
}

pim_inline uint4 VEC_CALL u4_fv(float x, float y, float z, float w)
{
    uint4 vec = { (u32)x, (u32)y, (u32)z, (u32)w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_fs(float s)
{
    u32 is = (u32)s;
    uint4 vec = { is, is, is, is };
    return vec;
}

pim_inline float4 VEC_CALL u4_f4(uint4 v)
{
    float4 f = { (float)v.x, (float)v.y, (float)v.z, (float)v.w };
    return f;
}

pim_inline uint4 VEC_CALL u4_xx(uint4 v)
{
    uint4 vec = { v.x, v.x, v.z, v.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_yy(uint4 v)
{
    uint4 vec = { v.y, v.y, v.z, v.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_yx(uint4 v)
{
    uint4 vec = { v.y, v.x, v.z, v.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_load(const u32* src)
{
    uint4 vec = { src[0], src[1], src[2], src[3] };
    return vec;
}

pim_inline void VEC_CALL u4_store(uint4 src, u32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
    dst[3] = src.w;
}

pim_inline uint4 VEC_CALL u4_add(uint4 lhs, uint4 rhs)
{
    uint4 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return vec;
}
pim_inline uint4 VEC_CALL u4_addvs(uint4 lhs, u32 rhs)
{
    uint4 vec = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs };
    return vec;
}
pim_inline uint4 VEC_CALL u4_addsv(u32 lhs, uint4 rhs)
{
    uint4 vec = { lhs + rhs.x, lhs + rhs.y, lhs + rhs.z, lhs + rhs.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_sub(uint4 lhs, uint4 rhs)
{
    uint4 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return vec;
}
pim_inline uint4 VEC_CALL u4_subvs(uint4 lhs, u32 rhs)
{
    uint4 vec = { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs };
    return vec;
}
pim_inline uint4 VEC_CALL u4_subsv(u32 lhs, uint4 rhs)
{
    uint4 vec = { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_mul(uint4 lhs, uint4 rhs)
{
    uint4 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
    return vec;
}
pim_inline uint4 VEC_CALL u4_mulvs(uint4 lhs, u32 rhs)
{
    uint4 vec = { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
    return vec;
}
pim_inline uint4 VEC_CALL u4_mulsv(u32 lhs, uint4 rhs)
{
    uint4 vec = { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w };
    return vec;
}

pim_inline uint4 VEC_CALL u4_div(uint4 lhs, uint4 rhs)
{
    uint4 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
    return vec;
}
pim_inline uint4 VEC_CALL u4_divvs(uint4 lhs, u32 rhs)
{
    uint4 vec = { lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs };
    return vec;
}
pim_inline uint4 VEC_CALL u4_divsv(u32 lhs, uint4 rhs)
{
    uint4 vec = { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z, lhs / rhs.w };
    return vec;
}

pim_inline bool4 VEC_CALL u4_eq(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_eqvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_eqsv(u32 lhs, uint4 rhs)
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

pim_inline bool4 VEC_CALL u4_neq(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_neqvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_neqsv(u32 lhs, uint4 rhs)
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

pim_inline bool4 VEC_CALL u4_lt(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_ltvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_ltsv(u32 lhs, uint4 rhs)
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

pim_inline bool4 VEC_CALL u4_gt(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_gtvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_gtsv(u32 lhs, uint4 rhs)
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

pim_inline bool4 VEC_CALL u4_lteq(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_lteqvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_lteqsv(u32 lhs, uint4 rhs)
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

pim_inline bool4 VEC_CALL u4_gteq(uint4 lhs, uint4 rhs)
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
pim_inline bool4 VEC_CALL u4_gteqvs(uint4 lhs, u32 rhs)
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
pim_inline bool4 VEC_CALL u4_gteqsv(u32 lhs, uint4 rhs)
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

pim_inline u32 VEC_CALL u4_sum4(uint4 v)
{
    return v.x + v.y + v.z + v.w;
}
pim_inline u32 VEC_CALL u4_sum3(uint4 v)
{
    return v.x + v.y + v.z;
}

pim_inline uint4 VEC_CALL u4_min(uint4 a, uint4 b)
{
    uint4 vec =
    {
        u1_min(a.x, b.x),
        u1_min(a.y, b.y),
        u1_min(a.z, b.z),
        u1_min(a.w, b.w),
    };
    return vec;
}
pim_inline uint4 VEC_CALL u4_minvs(uint4 a, u32 b)
{
    uint4 vec =
    {
        u1_min(a.x, b),
        u1_min(a.y, b),
        u1_min(a.z, b),
        u1_min(a.w, b),
    };
    return vec;
}
pim_inline uint4 VEC_CALL u4_minsv(u32 a, uint4 b)
{
    uint4 vec =
    {
        u1_min(a, b.x),
        u1_min(a, b.y),
        u1_min(a, b.z),
        u1_min(a, b.w),
    };
    return vec;
}

pim_inline uint4 VEC_CALL u4_max(uint4 a, uint4 b)
{
    uint4 vec =
    {
        u1_max(a.x, b.x),
        u1_max(a.y, b.y),
        u1_max(a.z, b.z),
        u1_max(a.z, b.w),
    };
    return vec;
}
pim_inline uint4 VEC_CALL u4_maxvs(uint4 a, u32 b)
{
    uint4 vec =
    {
        u1_max(a.x, b),
        u1_max(a.y, b),
        u1_max(a.z, b),
        u1_max(a.w, b),
    };
    return vec;
}
pim_inline uint4 VEC_CALL u4_maxsv(u32 a, uint4 b)
{
    uint4 vec =
    {
        u1_max(a, b.x),
        u1_max(a, b.y),
        u1_max(a, b.z),
        u1_max(a, b.w),
    };
    return vec;
}

pim_inline uint4 VEC_CALL u4_select(uint4 a, uint4 b, uint4 t)
{
    uint4 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
        t.z ? b.z : a.z,
        t.w ? b.w : a.w,
    };
    return c;
}
pim_inline uint4 VEC_CALL u4_selectvs(uint4 a, uint4 b, u32 t)
{
    uint4 c =
    {
        t ? b.x : a.x,
        t ? b.y : a.y,
        t ? b.z : a.z,
        t ? b.w : a.w,
    };
    return c;
}
pim_inline uint4 VEC_CALL u4_selectsv(u32 a, u32 b, uint4 t)
{
    uint4 c =
    {
        t.x ? b : a,
        t.y ? b : a,
        t.z ? b : a,
        t.w ? b : a,
    };
    return c;
}

pim_inline u32 VEC_CALL u4_hmin(uint4 v)
{
    return u1_min(
        u1_min(v.x, v.y),
        u1_min(v.z, v.w));
}

pim_inline u32 VEC_CALL u4_hmax(uint4 v)
{
    return u1_max(
        u1_max(v.x, v.y),
        u1_max(v.z, v.w));
}

pim_inline uint4 VEC_CALL u4_clamp(uint4 x, uint4 lo, uint4 hi)
{
    return u4_min(u4_max(x, lo), hi);
}
pim_inline uint4 VEC_CALL u4_clampvs(uint4 x, u32 lo, u32 hi)
{
    return u4_minvs(u4_maxvs(x, lo), hi);
}
pim_inline uint4 VEC_CALL u4_clampsv(u32 x, uint4 lo, uint4 hi)
{
    return u4_min(u4_maxsv(x, lo), hi);
}

pim_inline u32 VEC_CALL u4_dot4(uint4 a, uint4 b)
{
    return u4_sum4(u4_mul(a, b));
}
pim_inline u32 VEC_CALL u4_dot3(uint4 a, uint4 b)
{
    return u4_sum3(u4_mul(a, b));
}

pim_inline u32 VEC_CALL u4_lengthsq4(uint4 x)
{
    return u4_dot4(x, x);
}
pim_inline u32 VEC_CALL u4_lengthsq3(uint4 x)
{
    return u4_dot3(x, x);
}

pim_inline u32 VEC_CALL u4_length4(uint4 x)
{
    return (u32)(sqrt((double)u4_dot4(x, x)) + 0.5);
}
pim_inline u32 VEC_CALL u4_length3(uint4 x)
{
    return (u32)(sqrt((double)u4_dot3(x, x)) + 0.5);
}

pim_inline u32 VEC_CALL u4_distancesq4(uint4 a, uint4 b)
{
    return u4_lengthsq4(u4_sub(a, b));
}
pim_inline u32 VEC_CALL u4_distancesq3(uint4 a, uint4 b)
{
    return u4_lengthsq3(u4_sub(a, b));
}

pim_inline u32 VEC_CALL u4_distance4(uint4 a, uint4 b)
{
    return u4_length4(u4_sub(a, b));
}
pim_inline u32 VEC_CALL u4_distance3(uint4 a, uint4 b)
{
    return u4_length3(u4_sub(a, b));
}

pim_inline uint4 VEC_CALL u4_lerp(uint4 a, uint4 b, uint4 t)
{
    return u4_add(a, u4_mul(u4_sub(b, a), t));
}
pim_inline uint4 VEC_CALL u4_lerpvs(uint4 a, uint4 b, u32 t)
{
    return u4_add(a, u4_mulvs(u4_sub(b, a), t));
}
pim_inline uint4 VEC_CALL u4_lerpsv(u32 a, u32 b, uint4 t)
{
    return u4_addsv(a, u4_mulsv(b - a, t));
}

PIM_C_END
