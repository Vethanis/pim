#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/scalar.h"

// Attempts to fit a cubic polynomial to the given set of samples
// assumes domain and range of [0, 1]
float3 CubicFit(
    const float* pim_noalias ys,
    i32 len,
    i32 iterations,
    float* pim_noalias fitError);

float3 SqrticFit(
    const float* pim_noalias ys,
    i32 len,
    i32 iterations,
    float* pim_noalias fitError);

pim_inline float VEC_CALL CubicEval(float3 eq, float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    return eq.x * x + eq.y * x2 + eq.z * x3;
}

pim_inline float VEC_CALL SqrticEval(float3 eq, float x)
{
    float s1 = sqrtf(x);
    float s2 = sqrtf(s1);
    float s3 = sqrtf(s2);
    return eq.x * s1 + eq.y * s2 + eq.z * s3;
}

PIM_C_END
