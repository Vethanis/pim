#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float3_funcs.h"

// returns the value of the spherical gaussian when
// sampled at the point on a unit sphere given by normal vector 'dir'
pim_inline float3 VEC_CALL SGEval(SG_t sg, float3 dir)
{
    // gaussian in form of: e^(s * (x-1))
    // (a form of the normal distribution)
    float w = expf(sg.sharpness * (f3_dot(dir, sg.axis) - 1.0f));
    return f3_mulvs(sg.amplitude, w);
}

pim_inline float VEC_CALL SGBasisIntegral(SG_t sg)
{
    // integral of the surface of the unit sphere: tau
    // integral of e^(s * (x-1)) = (1 - e^-2s)/s
    // thus: (1 - e^-2s) * tau/s
    float e = 1.0f - expf(sg.sharpness * -2.0f);
    return (kTau * e) / sg.sharpness;
}

pim_inline float VEC_CALL SGBasisIntegralSq(SG_t sg)
{
    return (1.0f - expf(-4.0f * sg.sharpness)) / (4.0f * sg.sharpness);
}

// returns the total energy of the spherical gaussian
pim_inline float3 VEC_CALL SGIntegral(SG_t sg)
{
    return f3_mulvs(sg.amplitude, SGBasisIntegral(sg));
}

// approximates the total energy of the spherical gaussian
// 95% accurate with sharpness = 1.6
// 90% accurate with sharpness = 1.2
// 85% accurate with sharpness = 1.0
// 65% accurate with sharpness = 0.5
pim_inline float3 VEC_CALL SGFastIntegral(SG_t sg)
{
    return f3_mulvs(sg.amplitude, kTau / sg.sharpness);
}

// returns the irradiance (watts per square meter)
// received by a surface (with the given normal)
// from the spherical gaussian
pim_inline float3 SGIrradiance(SG_t sg, float3 normal)
{
    const float muDotN = f3_dot(sg.axis, normal);
    const float lambda = sg.sharpness;
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
    float y = (f1_abs(x0) <= x1) ? (n * n) / x : f1_saturate(muDotN);
    float normalizedIrradiance = scale * y + bias;
    return f3_mulvs(SGIntegral(sg), normalizedIrradiance);
}

// returns the diffuse lighting of a surface
// with the given normal and albedo
// from the given spherical gaussian.
pim_inline float3 SGDiffuse(SG_t sg, float3 normal, float3 albedo)
{
    const float scale = 1.0f / kPi;
    float3 brdf = f3_mulvs(albedo, scale);
    return f3_mul(SGIrradiance(sg, normal), brdf);
}

// SGSpecular not implemented; use a prefiltered cubemap for that.

// Averages in a new sample into the set of spherical gaussians
// that are being progressively fit to your data set.
// Ensure the sample directions are not correlated (do a uniform shuffle).
// Note that axis and sharpness are not modified, only amplitude.
void SGAccumulate(
    i32 iSample,        // sample sequence number
    float3 sampleDir,   // sample direction
    float3 sampleLight, // sample radiance
    SG_t* sgs,          // array of spherical gaussians to fit
    float* weights,     // working memory between accumulations. init to 0.
    i32 length);        // number of spherical gaussians

PIM_C_END
