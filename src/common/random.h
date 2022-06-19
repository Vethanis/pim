#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct Prng_s { uint4 state; } Prng;

Prng Prng_New(void);
Prng Prng_Get(void);
void Prng_Set(Prng rng);

pim_inline uint4 Prng_Next(Prng *const pim_noalias rng)
{
    // https://jcgt.org/published/0009/03/02/paper.pdf
    uint4 v = rng->state;
    v.x = v.x * 1664525u + 1013904223u;
    v.y = v.y * 1664525u + 1013904223u;
    v.z = v.z * 1664525u + 1013904223u;
    v.w = v.w * 1664525u + 1013904223u;
    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;
    v.x ^= v.x >> 16;
    v.y ^= v.y >> 16;
    v.z ^= v.z >> 16;
    v.w ^= v.w >> 16;
    v.x += v.y * v.w;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.w += v.y * v.z;
    rng->state = v;
    return v;
}

pim_inline u32 Prng_u32(Prng *const pim_noalias rng)
{
    return Prng_Next(rng).x;
}
pim_inline uint2 Prng_uint2(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    uint2 y = { v.x, v.y };
    return y;
}
pim_inline uint3 Prng_uint3(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    uint3 y = { v.x, v.y, v.z };
    return y;
}
pim_inline uint4 Prng_uint4(Prng *const pim_noalias rng)
{
    return Prng_Next(rng);
}

pim_inline bool Prng_bool(Prng *const pim_noalias rng)
{
    return Prng_u32(rng) & 1u;
}

pim_inline i32 Prng_i32(Prng *const pim_noalias rng)
{
    return (i32)(0x7fffffff & Prng_u32(rng));
}
pim_inline int2 Prng_int2(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    int2 y =
    {
        (i32)(v.x & 0x7fffffff),
        (i32)(v.y & 0x7fffffff),
    };
    return y;
}
pim_inline int3 Prng_int3(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    int3 y =
    {
        (i32)(v.x & 0x7fffffff),
        (i32)(v.y & 0x7fffffff),
        (i32)(v.z & 0x7fffffff),
    };
    return y;
}
pim_inline int4 Prng_int4(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    int4 y =
    {
        (i32)(v.x & 0x7fffffff),
        (i32)(v.y & 0x7fffffff),
        (i32)(v.z & 0x7fffffff),
        (i32)(v.w & 0x7fffffff),
    };
    return y;
}

pim_inline u64 Prng_u64(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    u64 y = v.x;
    y <<= 32;
    y |= v.y;
    return y;
}

pim_inline float VEC_CALL Prng_ToFloat(u32 x)
{
    return (x >> 8) * (1.0f / (1 << 24));
}
pim_inline float VEC_CALL Prng_f32(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    return Prng_ToFloat(v.x);
}
pim_inline float2 VEC_CALL Prng_float2(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    float2 y =
    {
        Prng_ToFloat(v.x),
        Prng_ToFloat(v.y),
    };
    return y;
}
pim_inline float3 VEC_CALL Prng_float3(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    float3 y =
    {
        Prng_ToFloat(v.x),
        Prng_ToFloat(v.y),
        Prng_ToFloat(v.z),
    };
    return y;
}
pim_inline float4 VEC_CALL Prng_float4(Prng *const pim_noalias rng)
{
    uint4 v = Prng_Next(rng);
    float4 y =
    {
        Prng_ToFloat(v.x),
        Prng_ToFloat(v.y),
        Prng_ToFloat(v.z),
        Prng_ToFloat(v.w),
    };
    return y;
}

PIM_C_END
