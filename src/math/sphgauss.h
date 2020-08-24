#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"

PIM_C_BEGIN

/*
    This file was made possible from the knowledge shared by the following people:
    * Matt Pettineo for his blog series on spherical gaussians
      - https://therealmjp.github.io/posts/new-blog-series-lightmap-baking-and-spherical-gaussians/
    * Matt Pettineo and Dave Neubelt for The Baking Lab
      - https://github.com/TheRealMJP/BakingLab
    * Stephen Hill for the polynomial fit of the integral of a spherical gaussian's diffuse irradiance about a direction
      - https://therealmjp.github.io/posts/sg-series-part-3-diffuse-lighting-from-an-sg-light-source/
    * Thomas Roughton for the progressive fitting method of lighting samples onto an array of spherical gaussians
      - https://torust.me/rendering/irradiance-caching/spherical-gaussians/2018/09/21/spherical-gaussians
*/

typedef enum
{
    SGDist_Hemi = 0,
    SGDist_Sphere,

    SGDist_COUNT
} SGDist;

// returns the value of a spherical gaussian basis when sampled by unit vector 'dir'
pim_inline float VEC_CALL SG_BasisEval(float4 axis, float4 dir)
{
    // gaussian in form of: e^(s * (x-1))
    // (a form of the normal distribution)
    float sharpness = axis.w;
    float cosTheta = f4_dot3(dir, axis);
    return expf(sharpness * (cosTheta - 1.0f));
}

// returns the value of the spherical gaussian when
// sampled at the point on a unit sphere given by normal vector 'dir'
pim_inline float4 VEC_CALL SG_Eval(float4 axis, float4 amplitude, float4 dir)
{
    return f4_mulvs(amplitude, SG_BasisEval(axis, dir));
}

pim_inline float4 VEC_CALL SGv_Eval(
    i32 length,
    const float4* pim_noalias axii,
    const float4* pim_noalias amplitudes,
    float4 normal)
{
    float4 sum = f4_0;
    for (i32 i = 0; i < length; ++i)
    {
        sum = f4_add(sum, SG_Eval(axii[i], amplitudes[i], normal));
    }
    return sum;
}

pim_inline float VEC_CALL SG_BasisIntegral(float sharpness)
{
    // integral of the surface of the unit hemisphere: tau
    // integral of e^(s * (x-1)) = (1 - e^-2s)/s
    float e = 1.0f - expf(sharpness * -2.0f);
    return kTau * (e / sharpness);
}

// returns the total energy of the spherical gaussian
pim_inline float4 VEC_CALL SG_Integral(float4 axis, float4 amplitude)
{
    return f4_mulvs(amplitude, SG_BasisIntegral(axis.w));
}

// This polynomial fit is originally authored by Stephen Hill.
// returns the irradiance (watts per square meter)
// received by a surface (with the given normal)
// from the spherical gaussian.
pim_inline float4 VEC_CALL SG_Irradiance(
    float4 axis,
    float4 amplitude,
    float4 normal)
{
    float muDotN = f4_dot3(axis, normal);
    float lambda = axis.w;

    const float c0 = 0.36f;
    const float c1 = 1.0f / (4.0f * 0.36f);

    float eml = expf(-lambda);
    float eml2 = eml * eml;
    float rl = 1.0f / lambda;

    float scale = 1.0f + 2.0f * eml2 - rl;
    float bias = (eml - eml2) * rl - eml2;

    float x = sqrtf(1.0f - scale);
    float x0 = c0 * muDotN;
    float x1 = c1 * x;

    float n = x0 + x1;

    float y = (f1_abs(x0) <= x1) ? (n * n) / x : f1_sat(muDotN);

    float normalizedIrradiance = scale * y + bias;

    return f4_mulvs(SG_Integral(axis, amplitude), normalizedIrradiance);
}

pim_inline float4 VEC_CALL SGv_Irradiance(
    i32 length,
    const float4* pim_noalias axii,
    const float4* pim_noalias amplitudes,
    float4 normal)
{
    float4 sum = f4_0;
    for (i32 i = 0; i < length; ++i)
    {
        sum = f4_add(sum, SG_Irradiance(axii[i], amplitudes[i], normal));
    }
    return sum;
}

// This method of fitting spherical gaussians originates from Thomas Roughton
// See https://torust.me/rendering/irradiance-caching/spherical-gaussians/2018/09/21/spherical-gaussians
// Averages in a new sample into the set of spherical gaussians
// that are being progressively fit to your data set.
// Ensure the sample directions are not correlated (do a uniform shuffle).
// Note that axis and sharpness are not modified, only amplitude.
void SG_Accumulate(
    float weight,
    float4 sampleDir,               // sample direction
    float4 sampleLight,             // sample radiance
    const float4* pim_noalias axii, // sg axii
    float4* pim_noalias amplitudes, // sg amplitudes to fit
    i32 length);                    // number of spherical gaussians

float SG_CalcSharpness(const float4* pim_noalias axii, i32 count);
void SG_Generate(float4* pim_noalias directions, i32 count, SGDist dist);

PIM_C_END
