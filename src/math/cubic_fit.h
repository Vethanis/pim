#pragma once

#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

// Attempts to fit a cubic polynomial to the given set of samples
float CubicFit(dataset_t data, fit_t* fit, i32 iterations);
float SqrticFit(dataset_t data, fit_t* fit, i32 iterations);
float TMapFit(dataset_t data, fit_t* fit, i32 iterations);
float PolyFit(dataset_t data, fit_t* fit, i32 iterations);

pim_inline float VEC_CALL CubicEval(float x, fit_t fit)
{
    return fit.value[0] * x + fit.value[1] * x * x + fit.value[2] * x * x * x;
}

pim_inline float VEC_CALL SqrticEval(float x, fit_t fit)
{
    float s1 = sqrtf(x);
    float s2 = sqrtf(s1);
    float s3 = sqrtf(s2);
    return fit.value[0] * s1 + fit.value[1] * s2 + fit.value[2] * s3;
}

pim_inline float VEC_CALL TMapEval(float x, fit_t fit)
{
    float a = fit.value[0];
    float b = fit.value[1];
    float c = fit.value[2];
    float d = fit.value[3];
    float e = fit.value[4];
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

pim_inline float VEC_CALL PolyEval(float x, fit_t fit)
{
    float x2 = x * x;
    float x3 = x * x * x;
    float nom = fit.value[1] * x + fit.value[2] * x2 + fit.value[3] * x3;
    float denom = fit.value[4] + fit.value[5] * x + fit.value[6] * x2 + fit.value[7] * x3;
    return nom / denom;
}

PIM_C_END
