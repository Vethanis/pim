#include "math/sphgauss.h"
#include "allocator/allocator.h"
#include "math/sampling.h"
#include "common/random.h"
#include "common/console.h"

pim_inline float4 VEC_CALL SampleDir(float2 Xi, SGDist dist)
{
    switch (dist)
    {
    default:
    case SGDist_Sphere:
        return SampleUnitSphere(Xi);
    case SGDist_Hemi:
        return SampleUnitHemisphere(Xi);
    }
}

void SG_Accumulate(
    float sampleWeight,
    float4 dir,
    float4 rad,
    const float4* pim_noalias axii,
    float4* pim_noalias amplitudes,
    i32 length)
{
    if (sampleWeight == 1.0f)
    {
        for (i32 i = 0; i < length; ++i)
        {
            amplitudes[i] = f4_0;
        }
    }

    float4 estimate = f4_0;
    for (i32 i = 0; i < length; ++i)
    {
        estimate = f4_add(estimate, SG_Eval(axii[i], amplitudes[i], dir));
    }

    for (i32 i = 0; i < length; ++i)
    {
        float4 axis = axii[i];
        float basis = SG_BasisEval(axis, dir);
        if (basis > 0.0f)
        {
            float4 amplitude = amplitudes[i];
            float weight = amplitude.w;
            weight = f1_lerp(weight, basis * basis, sampleWeight);
            float4 otherLobes = f4_sub(estimate, f4_mulvs(amplitude, basis));
            float4 thisLobe = f4_mulvs(f4_sub(rad, otherLobes), basis / weight);
            amplitude = f4_lerpvs(amplitude, thisLobe, sampleWeight);
            amplitude = f4_max(amplitude, f4_0);
            amplitude.w = weight;
            amplitudes[i] = amplitude;
        }
    }
}

float SG_CalcSharpness(const float4* pim_noalias axii, i32 count)
{
    // find cosine of the smallest half-angle between axii
    float minCosTheta = 1.0f;
    if (count > 0)
    {
        const float4 axii0 = axii[0];
        for (i32 i = 1; i < count; ++i)
        {
            float4 H = f4_normalize3(f4_add(axii[i], axii0));
            minCosTheta = f1_min(minCosTheta, f4_dot3(H, axii0));
        }
    }
    float sharpness = (logf(0.65f) * count) / (minCosTheta - 1.0f);
    return sharpness;
}

void SG_Generate(float4* pim_noalias axii, i32 count, SGDist dist)
{
    ASSERT(axii);
    ASSERT(count >= 0);

    for (i32 i = 0; i < count; ++i)
    {
        axii[i] = SampleDir(Hammersley2D(i, count), dist);
    }

    float sharpness = SG_CalcSharpness(axii, count);
    for (i32 i = 0; i < count; ++i)
    {
        axii[i].w = sharpness;
    }

    con_logf(LogSev_Info, "sg", "GI Directions: ");
    for (i32 i = 0; i < count; ++i)
    {
        con_logf(LogSev_Info, "sg", "%f, %f, %f, %f", axii[i].x, axii[i].y, axii[i].z, axii[i].w);
    }
}
