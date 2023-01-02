#pragma once

#include "math/types.h"
#include "math/scalar.h"
#include "common/atomics.h"
#include <string.h>

PIM_C_BEGIN

#ifndef ATOMIC_FLOAT_MAX
#   define ATOMIC_FLOAT_MAX (1e30f)
#endif // ATOMIC_FLOAT_MAX

pim_inline u32 VEC_CALL f1_tobits(float x)
{
    u32 y;
    memcpy(&y, &x, 4);
    return y;
}
pim_inline float VEC_CALL f1_frombits(u32 x)
{
    float y;
    memcpy(&y, &x, 4);
    return y;
}

pim_inline void VEC_CALL f1_add_atomic(volatile float* pA, float b)
{
    // this is just a CAS loop emulating an atomic float.
    // it is horribly slow, don't stick it in the inner-loop of a program.
    // thread-local work will almost always outperform atomics.
    volatile u32* dst = (volatile u32*)pA;
    u32 prev = load_u32(dst, MO_Relaxed);
    while (true)
    {
        float v = f1_frombits(prev) + b;
        // to avoid +-inf
        v = f1_min(v, ATOMIC_FLOAT_MAX);
        v = f1_max(v, -ATOMIC_FLOAT_MAX);
        u32 next = f1_tobits(v);
        if (cmpex_u32(dst, &prev, next, MO_Relaxed))
        {
            break;
        }
    }
}
pim_inline void VEC_CALL f2_add_atomic(volatile float2* pA, float2 b)
{
    f1_add_atomic(&pA->x, b.x);
    f1_add_atomic(&pA->y, b.y);
}
pim_inline void VEC_CALL f3_add_atomic(volatile float3* pA, float3 b)
{
    f1_add_atomic(&pA->x, b.x);
    f1_add_atomic(&pA->y, b.y);
    f1_add_atomic(&pA->z, b.z);
}
pim_inline void VEC_CALL f4_add_atomic(volatile float4* pA, float4 b)
{
    f1_add_atomic(&pA->x, b.x);
    f1_add_atomic(&pA->y, b.y);
    f1_add_atomic(&pA->z, b.z);
    f1_add_atomic(&pA->w, b.w);
}

pim_inline void VEC_CALL f1_lerp_atomic(volatile float* pA, float b, float t)
{
    volatile u32* dst = (volatile u32*)pA;
    u32 prev = load_u32(dst, MO_Relaxed);
    u32 next;
    do
    {
        next = f1_tobits(f1_lerp(f1_frombits(prev), b, t));
    } while (!cmpex_u32(dst, &prev, next, MO_Relaxed));
}
pim_inline void VEC_CALL f2_lerpvs_atomic(volatile float2* pA, float2 b, float t)
{
    f1_lerp_atomic(&pA->x, b.x, t);
    f1_lerp_atomic(&pA->y, b.y, t);
}
pim_inline void VEC_CALL f3_lerpvs_atomic(volatile float3* pA, float3 b, float t)
{
    f1_lerp_atomic(&pA->x, b.x, t);
    f1_lerp_atomic(&pA->y, b.y, t);
    f1_lerp_atomic(&pA->z, b.z, t);
}
pim_inline void VEC_CALL f4_lerpvs_atomic(volatile float4* pA, float4 b, float t)
{
    f1_lerp_atomic(&pA->x, b.x, t);
    f1_lerp_atomic(&pA->y, b.y, t);
    f1_lerp_atomic(&pA->z, b.z, t);
    f1_lerp_atomic(&pA->w, b.w, t);
}

PIM_C_END
