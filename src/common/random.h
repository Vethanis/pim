#pragma once

#include "common/int_types.h"
#include "math/vec_types.h"

namespace Random
{
    void Seed(u64 seed);
    void Seed();

    u32 NextU32();
    u32 NextU32(u32 hi);
    u32 NextU32(u32 lo, u32 hi);

    i32 NextI32();
    i32 NextI32(i32 hi);
    i32 NextI32(i32 lo, i32 hi);

    f32 NextF32();
    f32 NextF32(f32 hi);
    f32 NextF32(f32 lo, f32 hi);

    u64 NextU64();
    u64 NextU64(u64 hi);
    u64 NextU64(u64 lo, u64 hi);


    uint2 NextUint2();
    uint2 NextUint2(uint2 hi);
    uint2 NextUint2(uint2 lo, uint2 hi);

    uint3 NextUint3();
    uint3 NextUint3(uint3 hi);
    uint3 NextUint3(uint3 lo, uint3 hi);

    uint4 NextUint4();
    uint4 NextUint4(uint4 hi);
    uint4 NextUint4(uint4 lo, uint4 hi);


    float2 NextFloat2();
    float2 NextFloat2(float2 hi);
    float2 NextFloat2(float2 lo, float2 hi);

    float3 NextFloat3();
    float3 NextFloat3(float3 hi);
    float3 NextFloat3(float3 lo, float3 hi);

    float4 NextFloat4();
    float4 NextFloat4(float4 hi);
    float4 NextFloat4(float4 lo, float4 hi);
};
