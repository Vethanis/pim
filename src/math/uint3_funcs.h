#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

#define u3_0 u3_s(0)
#define u3_1 u3_s(1)
#define u3_2 u3_s(2)

pim_inline uint3 VEC_CALL u3_v(u32 x, u32 y, u32 z)
{
    uint3 vec = { x, y, z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_s(u32 s)
{
    uint3 vec = { s, s, s };
    return vec;
}

pim_inline uint3 VEC_CALL u3_fv(float x, float y, float z)
{
    uint3 vec = { (u32)x, (u32)y, (u32)z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_fs(float s)
{
    uint3 vec = { (u32)s, (u32)s, (u32)s };
    return vec;
}

pim_inline float3 VEC_CALL u3_f3(uint3 v)
{
    float3 f = { (float)v.x, (float)v.y, (float)v.z };
    return f;
}

pim_inline uint3 VEC_CALL u3_xx(uint3 v)
{
    uint3 vec = { v.x, v.x, v.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_yy(uint3 v)
{
    uint3 vec = { v.y, v.y, v.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_yx(uint3 v)
{
    uint3 vec = { v.y, v.x, v.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_load(const u32* src)
{
    uint3 vec = { src[0], src[1], src[2] };
    return vec;
}

pim_inline void VEC_CALL u3_store(uint3 src, u32* dst)
{
    dst[0] = src.x;
    dst[1] = src.y;
    dst[2] = src.z;
}

pim_inline uint3 VEC_CALL u3_add(uint3 lhs, uint3 rhs)
{
    uint3 vec = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    return vec;
}
pim_inline uint3 VEC_CALL u3_addvs(uint3 lhs, u32 rhs)
{
    uint3 vec = { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs };
    return vec;
}
pim_inline uint3 VEC_CALL u3_addsv(u32 lhs, uint3 rhs)
{
    uint3 vec = { lhs + rhs.x, lhs + rhs.y, lhs + rhs.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_sub(uint3 lhs, uint3 rhs)
{
    uint3 vec = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    return vec;
}
pim_inline uint3 VEC_CALL u3_subvs(uint3 lhs, u32 rhs)
{
    uint3 vec = { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs };
    return vec;
}
pim_inline uint3 VEC_CALL u3_subsv(u32 lhs, uint3 rhs)
{
    uint3 vec = { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_mul(uint3 lhs, uint3 rhs)
{
    uint3 vec = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
    return vec;
}
pim_inline uint3 VEC_CALL u3_mulvs(uint3 lhs, u32 rhs)
{
    uint3 vec = { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
    return vec;
}
pim_inline uint3 VEC_CALL u3_mulsv(u32 lhs, uint3 rhs)
{
    uint3 vec = { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_div(uint3 lhs, uint3 rhs)
{
    uint3 vec = { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z };
    return vec;
}
pim_inline uint3 VEC_CALL u3_divvs(uint3 lhs, u32 rhs)
{
    uint3 vec = { lhs.x / rhs, lhs.y / rhs, lhs.z / rhs };
    return vec;
}
pim_inline uint3 VEC_CALL u3_divsv(u32 lhs, uint3 rhs)
{
    uint3 vec = { lhs / rhs.x, lhs / rhs.y, lhs / rhs.z };
    return vec;
}

pim_inline uint3 VEC_CALL u3_neg(uint3 v)
{
    uint3 vec = { -v.x, -v.y, -v.z };
    return vec;
}

pim_inline bool3 VEC_CALL u3_eq(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x == rhs.x,
        lhs.y == rhs.y,
        lhs.z == rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_eqvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x == rhs,
        lhs.y == rhs,
        lhs.z == rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_eqsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs == rhs.x,
        lhs == rhs.y,
        lhs == rhs.z,
    };
    return vec;
}

pim_inline bool3 VEC_CALL u3_neq(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x != rhs.x,
        lhs.y != rhs.y,
        lhs.z != rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_neqvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x != rhs,
        lhs.y != rhs,
        lhs.z != rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_neqsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs != rhs.x,
        lhs != rhs.y,
        lhs != rhs.z,
    };
    return vec;
}

pim_inline bool3 VEC_CALL u3_lt(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x < rhs.x,
        lhs.y < rhs.y,
        lhs.z < rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_ltvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x < rhs,
        lhs.y < rhs,
        lhs.z < rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_ltsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs < rhs.x,
        lhs < rhs.y,
        lhs < rhs.z,
    };
    return vec;
}

pim_inline bool3 VEC_CALL u3_gt(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x > rhs.x,
        lhs.y > rhs.y,
        lhs.z > rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_gtvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x > rhs,
        lhs.y > rhs,
        lhs.z > rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_gtsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs > rhs.x,
        lhs > rhs.y,
        lhs > rhs.z,
    };
    return vec;
}

pim_inline bool3 VEC_CALL u3_lteq(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x <= rhs.x,
        lhs.y <= rhs.y,
        lhs.z <= rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_lteqvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x <= rhs,
        lhs.y <= rhs,
        lhs.z <= rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_lteqsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs <= rhs.x,
        lhs <= rhs.y,
        lhs <= rhs.z,
    };
    return vec;
}

pim_inline bool3 VEC_CALL u3_gteq(uint3 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs.x >= rhs.x,
        lhs.y >= rhs.y,
        lhs.z >= rhs.z,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_gteqvs(uint3 lhs, u32 rhs)
{
    bool3 vec =
    {
        lhs.x >= rhs,
        lhs.y >= rhs,
        lhs.z >= rhs,
    };
    return vec;
}
pim_inline bool3 VEC_CALL u3_gteqsv(u32 lhs, uint3 rhs)
{
    bool3 vec =
    {
        lhs >= rhs.x,
        lhs >= rhs.y,
        lhs >= rhs.z,
    };
    return vec;
}

pim_inline u32 VEC_CALL u3_sum(uint3 v)
{
    return v.x + v.y + v.z;
}

pim_inline uint3 VEC_CALL u3_min(uint3 a, uint3 b)
{
    uint3 vec =
    {
        u1_min(a.x, b.x),
        u1_min(a.y, b.y),
        u1_min(a.z, b.z),
    };
    return vec;
}
pim_inline uint3 VEC_CALL u3_minvs(uint3 a, u32 b)
{
    uint3 vec =
    {
        u1_min(a.x, b),
        u1_min(a.y, b),
        u1_min(a.z, b),
    };
    return vec;
}
pim_inline uint3 VEC_CALL u3_minsv(u32 a, uint3 b)
{
    uint3 vec =
    {
        u1_min(a, b.x),
        u1_min(a, b.y),
        u1_min(a, b.z),
    };
    return vec;
}

pim_inline uint3 VEC_CALL u3_max(uint3 a, uint3 b)
{
    uint3 vec =
    {
        u1_max(a.x, b.x),
        u1_max(a.y, b.y),
        u1_max(a.z, b.z),
    };
    return vec;
}
pim_inline uint3 VEC_CALL u3_maxvs(uint3 a, u32 b)
{
    uint3 vec =
    {
        u1_max(a.x, b),
        u1_max(a.y, b),
        u1_max(a.z, b),
    };
    return vec;
}
pim_inline uint3 VEC_CALL u3_maxsv(u32 a, uint3 b)
{
    uint3 vec =
    {
        u1_max(a, b.x),
        u1_max(a, b.y),
        u1_max(a, b.z),
    };
    return vec;
}

pim_inline uint3 VEC_CALL u3_select(uint3 a, uint3 b, uint3 t)
{
    uint3 c =
    {
        t.x ? b.x : a.x,
        t.y ? b.y : a.y,
        t.z ? b.z : a.z,
    };
    return c;
}
pim_inline uint3 VEC_CALL u3_selectvs(uint3 a, uint3 b, u32 t)
{
    uint3 c =
    {
        t ? b.x : a.x,
        t ? b.y : a.y,
        t ? b.z : a.z,
    };
    return c;
}
pim_inline uint3 VEC_CALL u3_selectsv(u32 a, u32 b, uint3 t)
{
    uint3 c =
    {
        t.x ? b : a,
        t.y ? b : a,
        t.z ? b : a,
    };
    return c;
}

pim_inline u32 VEC_CALL u3_hmin(uint3 v)
{
    return u1_min(u1_min(v.x, v.y), v.z);
}

pim_inline u32 VEC_CALL u3_hmax(uint3 v)
{
    return u1_max(u1_max(v.x, v.y), v.z);
}

pim_inline uint3 VEC_CALL u3_clamp(uint3 x, uint3 lo, uint3 hi)
{
    return u3_min(u3_max(x, lo), hi);
}
pim_inline uint3 VEC_CALL u3_clampvs(uint3 x, u32 lo, u32 hi)
{
    return u3_minvs(u3_maxvs(x, lo), hi);
}
pim_inline uint3 VEC_CALL u3_clampsv(u32 x, uint3 lo, uint3 hi)
{
    return u3_min(u3_maxsv(x, lo), hi);
}

pim_inline u32 VEC_CALL u3_dot(uint3 a, uint3 b)
{
    return u3_sum(u3_mul(a, b));
}

pim_inline u32 VEC_CALL u3_length(uint3 x)
{
    return (u32)(sqrt((double)u3_dot(x, x)) + 0.5);
}

pim_inline u32 VEC_CALL u3_distance(uint3 a, uint3 b)
{
    return u3_length(u3_sub(a, b));
}
pim_inline u32 VEC_CALL u3_distancevs(uint3 a, u32 b)
{
    return u3_length(u3_subvs(a, b));
}
pim_inline u32 VEC_CALL u3_distancesv(u32 a, uint3 b)
{
    return u3_length(u3_subsv(a, b));
}

pim_inline u32 VEC_CALL u3_lengthsq(uint3 x)
{
    return u3_dot(x, x);
}

pim_inline u32 VEC_CALL u3_distancesq(uint3 a, uint3 b)
{
    return u3_lengthsq(u3_sub(a, b));
}

pim_inline uint3 VEC_CALL u3_lerp(uint3 a, uint3 b, uint3 t)
{
    return u3_add(a, u3_mul(u3_sub(b, a), t));
}
pim_inline uint3 VEC_CALL u3_lerpvs(uint3 a, uint3 b, u32 t)
{
    return u3_add(a, u3_mulvs(u3_sub(b, a), t));
}
pim_inline uint3 VEC_CALL u3_lerpsv(u32 a, u32 b, uint3 t)
{
    return u3_addsv(a, u3_mulsv(b - a, t));
}

PIM_C_END
