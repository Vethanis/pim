#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdbool.h>
#include <math.h>

#ifndef kPi
#define kPi                 3.141592653f
#define kTau                6.283185307f
#define kRadiansPerDegree   (kTau / 360.0f)
#define kDegreesPerRadian   (360.0f / kTau)
#endif // kPi

#define math_inline __forceinline

math_inline float VEC_CALL f32_min(float a, float b)
{
    return a < b ? a : b;
}

math_inline float VEC_CALL f32_max(float a, float b)
{
    return a > b ? a : b;
}

math_inline float VEC_CALL f32_radians(float x)
{
    return x * kRadiansPerDegree;
}

math_inline float VEC_CALL f32_degrees(float x)
{
    return x * kDegreesPerRadian;
}

PIM_C_END
