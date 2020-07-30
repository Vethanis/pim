#pragma once

#include "math/types.h"
#include "math/float4_funcs.h"
#include "math/float4x4_funcs.h"

pim_inline box_t VEC_CALL box_new(float4 lo, float4 hi)
{
    box_t box = { lo, hi };
    return box;
}

pim_inline box_t VEC_CALL box_empty(void)
{
    return box_new(f4_s(1 << 20), f4_s(-(1 << 20)));
}

pim_inline float4 VEC_CALL box_center(box_t box)
{
    return f4_lerpvs(box.lo, box.hi, 0.5f);
}

pim_inline float4 VEC_CALL box_size(box_t box)
{
    return f4_sub(box.hi, box.lo);
}

pim_inline float4 VEC_CALL box_extents(box_t box)
{
    return f4_mulvs(box_size(box), 0.5f);
}

pim_inline bool VEC_CALL box_contains(box_t box, float4 pt)
{
    return b4_all3(f4_gteq(pt, box.lo)) && b4_all3(f4_lteq(pt, box.hi));
}

pim_inline float VEC_CALL box_area(box_t box)
{
    float4 size = box_size(box);
    return size.x * size.y * size.z;
}

pim_inline box_t VEC_CALL box_from_pts(const float4* pim_noalias pts, i32 length)
{
    if (length == 0)
    {
        return box_new(f4_0, f4_0);
    }
    float4 lo = f4_s(1 << 20);
    float4 hi = f4_s(-(1 << 20));
    for (i32 i = 0; i < length; ++i)
    {
        float4 pt = pts[i];
        lo = f4_min(lo, pt);
        hi = f4_max(hi, pt);
    }
    return box_new(lo, hi);
}

pim_inline box_t VEC_CALL box_union(box_t lhs, box_t rhs)
{
    return box_new(f4_min(lhs.lo, rhs.lo), f4_max(lhs.hi, rhs.hi));
}

pim_inline box_t VEC_CALL box_intersect(box_t lhs, box_t rhs)
{
    float4 lo = f4_min(lhs.lo, rhs.lo);
    float4 hi = f4_max(lhs.hi, rhs.hi);
    float4 mid = f4_lerpvs(lo, hi, 0.5f);
    bool4 inverted = f4_gt(lo, hi);
    lo = f4_select(lo, mid, inverted);
    hi = f4_select(hi, mid, inverted);
    return box_new(lo, hi);
}

pim_inline box_t VEC_CALL box_transform(float4x4 matrix, box_t box)
{
    float4 center = box_center(box);
    float4 extents = f4_sub(box.hi, center);
    center = f4x4_mul_pt(matrix, center);
    extents = f4x4_mul_extents(matrix, extents);
    float4 lo = f4_sub(center, extents);
    float4 hi = f4_add(center, extents);
    return box_new(lo, hi);
}
