#pragma once

#include "math/pcg.h"
#include "math/scalar.h"

PIM_C_BEGIN

void Random_Init(void);

Prng Prng_New(void);
Prng* Prng_Get(void);

pim_inline void VEC_CALL Prng_Next1(Prng* pim_noalias rng)
{
    u32 v = rng->state.x;
    v = Pcg1(v);
    rng->state.x = v;
}
pim_inline void VEC_CALL Prng_Next2(Prng* pim_noalias rng)
{
    uint2 v = Pcg2(rng->state.x, rng->state.y);
    rng->state.x = v.x;
    rng->state.y = v.y;
}
pim_inline void VEC_CALL Prng_Next3(Prng* pim_noalias rng)
{
    uint3 v = Pcg3(rng->state.x, rng->state.y, rng->state.z);
    rng->state.x = v.x;
    rng->state.y = v.y;
    rng->state.z = v.z;
}
pim_inline void VEC_CALL Prng_Next4(Prng* pim_noalias rng)
{
    rng->state = Pcg4(rng->state.x, rng->state.y, rng->state.z, rng->state.w);
}

pim_inline u32 VEC_CALL Prng_u32(Prng* pim_noalias rng)
{
    Prng_Next1(rng);
    return rng->state.x;
}
pim_inline uint2 VEC_CALL Prng_uint2(Prng* pim_noalias rng)
{
    Prng_Next2(rng);
    return (uint2) { rng->state.x, rng->state.y };
}
pim_inline uint3 VEC_CALL Prng_uint3(Prng* pim_noalias rng)
{
    Prng_Next3(rng);
    return (uint3) { rng->state.x, rng->state.y, rng->state.z };
}
pim_inline uint4 VEC_CALL Prng_uint4(Prng* pim_noalias rng)
{ 
    Prng_Next4(rng);
    return rng->state;
}

pim_inline bool VEC_CALL Prng_bool(Prng* pim_noalias rng)
{
    return (Prng_u32(rng) >> 31) != 0;
}

pim_inline i32 VEC_CALL Prng_i32(Prng* pim_noalias rng)
{
    Prng_Next1(rng);
    return (i32)(rng->state.x & 0x7fffffff);
}
pim_inline int2 VEC_CALL Prng_int2(Prng* pim_noalias rng)
{
    Prng_Next2(rng);
    return (int2)
    {
        (i32)(rng->state.x & 0x7fffffff),
        (i32)(rng->state.y & 0x7fffffff),
    };
}
pim_inline int3 VEC_CALL Prng_int3(Prng* pim_noalias rng)
{
    Prng_Next3(rng);
    return (int3)
    {
        (i32)(rng->state.x & 0x7fffffff),
        (i32)(rng->state.y & 0x7fffffff),
        (i32)(rng->state.z & 0x7fffffff),
    };
}
pim_inline int4 VEC_CALL Prng_int4(Prng* pim_noalias rng)
{
    Prng_Next4(rng);
    return (int4)
    {
        (i32)(rng->state.x & 0x7fffffff),
        (i32)(rng->state.y & 0x7fffffff),
        (i32)(rng->state.z & 0x7fffffff),
        (i32)(rng->state.w & 0x7fffffff),
    };
}

pim_inline u64 VEC_CALL Prng_u64(Prng* pim_noalias rng)
{
    Prng_Next2(rng);
    u64 y = rng->state.x;
    y = y << 32;
    y = y | rng->state.y;
    return y;
}

pim_inline float VEC_CALL Prng_ToFloat(u32 x)
{
    return (x >> 8) * (1.0f / (1 << 24));
}
pim_inline float VEC_CALL Prng_f32(Prng* pim_noalias rng)
{
    Prng_Next1(rng);
    return Prng_ToFloat(rng->state.x);
}
pim_inline float2 VEC_CALL Prng_float2(Prng* pim_noalias rng)
{
    Prng_Next2(rng);
    return (float2)
    {
        Prng_ToFloat(rng->state.x),
        Prng_ToFloat(rng->state.y),
    };
}
pim_inline float3 VEC_CALL Prng_float3(Prng* pim_noalias rng)
{
    Prng_Next3(rng);
    return (float3)
    {
        Prng_ToFloat(rng->state.x),
        Prng_ToFloat(rng->state.y),
        Prng_ToFloat(rng->state.z),
    };
}
pim_inline float4 VEC_CALL Prng_float4(Prng* pim_noalias rng)
{
    Prng_Next4(rng);
    return (float4)
    {
        Prng_ToFloat(rng->state.x),
        Prng_ToFloat(rng->state.y),
        Prng_ToFloat(rng->state.z),
        Prng_ToFloat(rng->state.w),
    };
}

PIM_C_END
