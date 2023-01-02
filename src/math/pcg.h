#pragma once

#include "math/types.h"

PIM_C_BEGIN

// Title:
// - PCG: A Family of Simple Fast
//   Space - Efficient Statistically
//   Good Algorithms
//   for Random Number Generation
// Author:
//  - Melissa E. O’Neill
// Link:
//  - https://www.pcg-random.org
pim_inline u32 VEC_CALL Pcg1_Lcg(u32 v)
{
    return v * 747796405u + 2891336453u;
}
pim_inline u32 VEC_CALL Pcg1_Permute(u32 v)
{
    v = ((v >> ((v >> 28) + 4u)) ^ v) * 277803737u;
    v = (v >> 22) ^ v;
    return v;
}
pim_inline u32 VEC_CALL Pcg1(u32 v)
{
    v = v * 747796405u + 2891336453u;
    v = ((v >> ((v >> 28) + 4u)) ^ v) * 277803737u;
    v = (v >> 22) ^ v;
    return v;
}
u32 VEC_CALL Pcg1_Bytes(const void* pim_noalias x, i32 bytes, u32 hash);
u32 VEC_CALL Pcg1_String(const char* pim_noalias x, u32 hash);

// Title:
// - Hash Functions for GPU Rendering
// Authors:
//  - Mark Jarzynski
//  - Marc Olano
// Link:
//  - https://jcgt.org/published/0009/03/02
pim_inline uint2 VEC_CALL Pcg2_Lcg(uint2 v)
{
    v.x = v.x * 1664525u + 1013904223u;
    v.y = v.y * 1664525u + 1013904223u;
    return v;
}
pim_inline uint2 VEC_CALL Pcg2_Permute(uint2 v)
{
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v.x ^= v.x >> 16;
    v.y ^= v.y >> 16;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    return v;
}
pim_inline uint2 VEC_CALL Pcg2(u32 x, u32 y)
{
    x = x * 1664525u + 1013904223u;
    y = y * 1664525u + 1013904223u;
    x += y * 1664525u;
    y += x * 1664525u;
    x ^= x >> 16;
    y ^= y >> 16;
    x += y * 1664525u;
    y += x * 1664525u;
    return (uint2) { x, y };
}
uint2 VEC_CALL Pcg2_Bytes(const void* pim_noalias x, i32 bytes, uint2 hash);
uint2 VEC_CALL Pcg2_String(const char* pim_noalias x, uint2 hash);


// Title:
// - Hash Functions for GPU Rendering
// Authors:
//  - Mark Jarzynski
//  - Marc Olano
// Link:
//  - https://jcgt.org/published/0009/03/02
pim_inline uint3 VEC_CALL Pcg3_Lcg(uint3 v)
{
    v.x = v.x * 1664525u + 1013904223u;
    v.y = v.y * 1664525u + 1013904223u;
    v.z = v.z * 1664525u + 1013904223u;
    return v;
}
pim_inline uint3 VEC_CALL Pcg3_Permute(uint3 v)
{
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v.x ^= v.x >> 16;
    v.y ^= v.y >> 16;
    v.z ^= v.z >> 16;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}
pim_inline uint3 VEC_CALL Pcg3(u32 x, u32 y, u32 z)
{
    x = x * 1664525u + 1013904223u;
    y = y * 1664525u + 1013904223u;
    z = z * 1664525u + 1013904223u;
    x += y * z;
    y += z * x;
    z += x * y;
    x ^= x >> 16;
    y ^= y >> 16;
    z ^= z >> 16;
    x += y * z;
    y += z * x;
    z += x * y;
    return (uint3) { x, y, z };
}
uint3 VEC_CALL Pcg3_Bytes(const void* pim_noalias x, i32 bytes, uint3 hash);
uint3 VEC_CALL Pcg3_String(const char* pim_noalias x, uint3 hash);

// Title:
// - Hash Functions for GPU Rendering
// Authors:
//  - Mark Jarzynski
//  - Marc Olano
// Link:
//  - https://jcgt.org/published/0009/03/02
pim_inline uint4 VEC_CALL Pcg4_Lcg(uint4 v)
{
    v.x = v.x * 1664525u + 1013904223u;
    v.y = v.y * 1664525u + 1013904223u;
    v.z = v.z * 1664525u + 1013904223u;
    v.w = v.w * 1664525u + 1013904223u;
    return v;
}
pim_inline uint4 VEC_CALL Pcg4_Permute(uint4 v)
{
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
    return v;
}
pim_inline uint4 VEC_CALL Pcg4(u32 x, u32 y, u32 z, u32 w)
{
    x = x * 1664525u + 1013904223u;
    y = y * 1664525u + 1013904223u;
    z = z * 1664525u + 1013904223u;
    w = w * 1664525u + 1013904223u;
    x += y * w;
    y += z * x;
    z += x * y;
    w += y * z;
    x ^= x >> 16;
    y ^= y >> 16;
    z ^= z >> 16;
    w ^= w >> 16;
    x += y * w;
    y += z * x;
    z += x * y;
    w += y * z;
    return (uint4) { x, y, z, w };
}
uint4 VEC_CALL Pcg4_Bytes(const void* pim_noalias x, i32 bytes, uint4 hash);
uint4 VEC_CALL Pcg4_String(const char* pim_noalias x, uint4 hash);

PIM_C_END
