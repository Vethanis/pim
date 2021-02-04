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

static float FitBasis(float target, i32 count)
{
    float fit = 1.0f;
    float err = f1_abs(target - (SG_BasisIntegral(fit) * count));
    float t = err;
    while (err > kMilli)
    {
        for (i32 j = 0; j < 22; ++j)
        {
            const float step = 1.0f / (1 << j);
            t = fmodf(t + kGoldenRatio, 1.0f);
            float subFit = fit + f1_lerp(-1.0f, 1.0f, t) * step;
            subFit = f1_max(kEpsilon, subFit);
            float subErr = f1_abs(target - (SG_BasisIntegral(subFit) * count));
            if (subErr < err)
            {
                fit = subFit;
                err = subErr;
            }
        }
    }
    return fit;
}

static float FitError(const float4* pim_noalias axii, i32 count, float s, float target)
{
    if (count > 0)
    {
        float integral = SG_BasisIntegral(s) * count;
        float avgOverlap = 0.0f;
        for (i32 i = 0; i < count; ++i)
        {
            float4 v1 = axii[i];
            v1.w = s;
            float overlap = 0.0f;
            for (i32 j = 0; j < count; ++j)
            {
                if (i != j)
                {
                    float4 v2 = axii[j];
                    v2.w = s;
                    overlap += SG_BasisEval(v1, v2);
                }
            }
            avgOverlap += overlap;
        }
        avgOverlap /= count;
        return f1_abs(target - integral) + f1_abs(avgOverlap);
    }
    return 0.0f;
}

float SG_CalcSharpness(const float4* pim_noalias axii, i32 count)
{
    const float target = kTau;
    float fit = FitBasis(target, count);
    float err = FitError(axii, count, fit, target);

    float t = err;
    for (i32 i = 0; i < 100; ++i)
    {
        for (i32 j = 0; j < 22; ++j)
        {
            const float step = 1.0f / (1 << j);
            t = fmodf(t + kGoldenRatio, 1.0f);
            float subFit = fit + f1_lerp(-1.0f, 1.0f, t) * step;
            subFit = f1_max(kEpsilon, subFit);
            float subErr = FitError(axii, count, subFit, target);
            if (subErr < err)
            {
                fit = subFit;
                err = subErr;
            }
        }
    }

    return fit;
}

void SG_Generate(float4* pim_noalias axii, i32 count, SGDist dist)
{
    ASSERT(axii);
    ASSERT(count >= 0);

    if (count == 5)
    {
        axii[0] = f4_v(0.0f, 0.0f, 1.0f, 0.0f);
        axii[1] = f4_normalize3(f4_v(1.0f, 1.0f, 1.0f, 0.0f));
        axii[2] = f4_normalize3(f4_v(-1.0f, 1.0f, 1.0f, 0.0f));
        axii[3] = f4_normalize3(f4_v(1.0f, -1.0f, 1.0f, 0.0f));
        axii[4] = f4_normalize3(f4_v(-1.0f, -1.0f, 1.0f, 0.0f));
    }
    else
    {
        for (i32 i = 0; i < count; ++i)
        {
            axii[i] = SampleDir(Hammersley2D(i, count), dist);
        }
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
