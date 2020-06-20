#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"

PIM_C_BEGIN

typedef enum
{
    SGDist_Hemi = 0,
    SGDist_Sphere,

    SGDist_COUNT
} SGDist;

// returns the pdf of a spherical gaussian when sampled by unit vector 'dir'
pim_inline float VEC_CALL SG_BasisEval(float4 axis, float4 dir)
{
    // gaussian in form of: e^(s * (x-1))
    // (a form of the normal distribution)
    float sharpness = axis.w;
    float cosTheta = f1_sat(f4_dot3(dir, axis));
    return expf(sharpness * (cosTheta - 1.0f));
}

// returns the value of the spherical gaussian when
// sampled at the point on a unit sphere given by normal vector 'dir'
pim_inline float4 VEC_CALL SG_Eval(float4 axis, float4 amplitude, float4 dir)
{
    return f4_mulvs(amplitude, SG_BasisEval(axis, dir));
}

pim_inline float VEC_CALL SG_BasisIntegral(float sharpness)
{
    // integral of the surface of the unit sphere: tau
    // integral of e^(s * (x-1)) = (1 - e^-2s)/s
    float e = 1.0f - expf(sharpness * -2.0f);
    return kTau * (e / sharpness);
}

pim_inline float VEC_CALL SG_BasisIntegralSq(float sharpness)
{
    return (1.0f - expf(-4.0f * sharpness)) / (4.0f * sharpness);
}

// returns the total energy of the spherical gaussian
pim_inline float4 VEC_CALL SG_Integral(float4 axis, float4 amplitude)
{
    return f4_mulvs(amplitude, SG_BasisIntegral(axis.w));
}

// returns the irradiance (watts per square meter)
// received by a surface (with the given normal)
// from the spherical gaussian
pim_inline float4 VEC_CALL SG_Irradiance(float4 axis, float4 amplitude, float4 normal)
{
    float muDotN = f1_sat(f4_dot3(axis, normal));
    float lambda = axis.w;
    float eml = expf(-lambda);
    float eml2 = eml * eml;
    float rl = 1.0f / lambda;
    float scale = 1.0f + 2.0f * eml2 - rl;
    float bias = (eml - eml2) * rl - eml2;
    float x = sqrtf(1.0f - scale);
    float x0 = 0.36f * muDotN;
    float x1 = x / (4.0f * 0.36f);
    float n = x0 + x1;
    float y = (f1_abs(x0) <= x1) ? (n * n) / x : muDotN;
    return f4_mulvs(SG_Integral(axis, amplitude), scale * y + bias);
}

// Averages in a new sample into the set of spherical gaussians
// that are being progressively fit to your data set.
// Ensure the sample directions are not correlated (do a uniform shuffle).
// Note that axis and sharpness are not modified, only amplitude.
void SG_Accumulate(
    i32 iSample,            // sample sequence number
    float4 sampleDir,       // sample direction
    float4 sampleLight,     // sample radiance
    const float4* axii,     // sg axii
    float4* amplitudes,     // sg amplitudes to fit
    float* weights,         // lobe weights scratch memory
    const float* integrals, // basis sq integrals from SG_Generate
    i32 length);            // number of spherical gaussians

void SG_Generate(float4* directions, float* integrals, i32 count, SGDist dist);

PIM_C_END
