#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float4_funcs.h"

// returns the value of the spherical gaussian when
// sampled at the point on a unit sphere given by normal vector 'dir'
pim_inline float VEC_CALL SG_BasisEval(SG_t sg, float4 dir)
{
    // gaussian in form of: e^(s * (x-1))
    // (a form of the normal distribution)
    float x = f1_sat(f4_dot3(dir, sg.axis));
    float s = sg.axis.w;
    float w = expf(s * (x - 1.0f));
    return w;
}

// returns the value of the spherical gaussian when
// sampled at the point on a unit sphere given by normal vector 'dir'
pim_inline float4 VEC_CALL SG_Eval(SG_t sg, float4 dir)
{
    return f4_mulvs(sg.amplitude, SG_BasisEval(sg, dir));
}

pim_inline float VEC_CALL SG_BasisIntegral(SG_t sg)
{
    // integral of the surface of the unit sphere: tau
    // integral of e^(s * (x-1)) = (1 - e^-2s)/s
    float s = sg.axis.w;
    float e = 1.0f - expf(s * -2.0f);
    return kTau * (e / s);
}

pim_inline float VEC_CALL SG_BasisIntegralSq(SG_t sg)
{
    return (1.0f - expf(-4.0f * sg.axis.w)) / (4.0f * sg.axis.w);
}

// returns the total energy of the spherical gaussian
pim_inline float4 VEC_CALL SG_Integral(SG_t sg)
{
    return f4_mulvs(sg.amplitude, SG_BasisIntegral(sg));
}

// returns the irradiance (watts per square meter)
// received by a surface (with the given normal)
// from the spherical gaussian
pim_optimize
pim_inline float4 SG_Irradiance(SG_t sg, float4 normal)
{
    const float muDotN = f1_sat(f4_dot3(sg.axis, normal));
    const float lambda = sg.axis.w;
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
    return f4_mulvs(SG_Integral(sg), scale * y + bias);
}

// Averages in a new sample into the set of spherical gaussians
// that are being progressively fit to your data set.
// Ensure the sample directions are not correlated (do a uniform shuffle).
// Note that axis and sharpness are not modified, only amplitude.
void VEC_CALL SG_Accumulate(
    i32 iSample,            // sample sequence number
    float4 sampleDir,       // sample direction
    float4 sampleLight,     // sample radiance
    SG_t* sgs,              // array of spherical gaussians to fit
    float* weights,         // lobe weights scratch memory
    const float* integrals, // basis sq integrals from SG_Generate
    i32 length);            // number of spherical gaussians

typedef enum
{
    SGDist_Hemi = 0,
    SGDist_Sphere,

    SGDist_COUNT
} SG_Dist;

void SG_Generate(SG_t* sgs, float* integrals, i32 count, SG_Dist dist);

PIM_C_END
