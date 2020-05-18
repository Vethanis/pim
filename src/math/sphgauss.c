#include "math/sphgauss.h"
#include "allocator/allocator.h"
#include "math/sampling.h"
#include "common/random.h"

static float4 VEC_CALL SampleDir(float2 Xi, SG_Dist dist)
{
    float4 dir;
    switch (dist)
    {
    default:
    case SGDist_Sphere:
        dir = SampleUnitSphere(Xi);
        break;
    case SGDist_Hemi:
        dir = SampleUnitHemisphere(Xi);
        break;
    }
    return f4_normalize3(dir);
}

pim_optimize
void VEC_CALL SG_Accumulate(
    i32 iSample,
    float4 sampleDir,
    float4 sampleLight,
    SG_t* sgs,
    i32 length)
{
    const float sampleScale = 1.0f / (iSample + 1.0f);
    const i32 pushBytes = sizeof(float) * length;
    float* sampleWeights = pim_pusha(pushBytes);

    float4 est = f4_0;
    for (i32 i = 0; i < length; ++i)
    {
        const SG_t sg = sgs[i];
        float cosTheta = f4_dot3(sg.axis, sampleDir);
        float sampleWeight = expf(sg.sharpness * (cosTheta - 1.0f));
        est = f4_add(est, f4_mulvs(sg.amplitude, sampleWeight));
        sampleWeights[i] = sampleWeight;
    }

    for (i32 i = 0; i < length; ++i)
    {
        float sampleWeight = sampleWeights[i];
        if (sampleWeight == 0.0f)
        {
            continue;
        }
        SG_t sg = sgs[i];

        sg.lobeWeight += ((sampleWeight * sampleWeight) - sg.lobeWeight) * sampleScale;
        float integral = f1_max(sg.lobeWeight, sg.basisSqIntegral);
        float4 otherLight = f4_sub(est, f4_mulvs(sg.amplitude, sampleWeight));
        float4 newEst = f4_mulvs(
            f4_sub(sampleLight, otherLight),
            sampleWeight / integral);
        float4 diff = f4_mulvs(f4_sub(newEst, sg.amplitude), sampleScale);
        sg.amplitude = f4_add(sg.amplitude, diff);
        // non-negative fit
        sg.amplitude = f4_max(sg.amplitude, f4_0);

        sgs[i] = sg;
    }

    pim_popa(pushBytes);
}

pim_optimize
void SG_Generate(SG_t* sgs, i32 count, SG_Dist dist)
{
    ASSERT(sgs);
    ASSERT(count >= 0);

    prng_t rng = prng_create();

    for (i32 i = 0; i < count; ++i)
    {
        sgs[i].axis = SampleDir(Hammersley2D(i, count), dist);
    }

    // shuffle directions
    for (i32 i = 0; i < count; ++i)
    {
        i32 j = prng_i32(&rng) % count;
        ASSERT(j >= 0);
        ASSERT(j < count);
        SG_t tmp = sgs[i];
        sgs[i] = sgs[j];
        sgs[j] = tmp;
    }

    float minCosTheta = 1.0f;
    for (i32 i = 1; i < count; ++i)
    {
        float4 H = f4_normalize3(f4_add(sgs[i].axis, sgs[0].axis));
        minCosTheta = f1_min(minCosTheta, f4_dot3(H, sgs[0].axis));
    }

    float sharpness = (logf(0.65f) * count) / (minCosTheta - 1.0f);
    for (i32 i = 0; i < count; ++i)
    {
        sgs[i].sharpness = sharpness;
        sgs[i].basisIntegral = SG_BasisIntegral(sgs[i]);
        sgs[i].basisSqIntegral = 0.0f;
        sgs[i].amplitude = f4_0;
        sgs[i].lobeWeight = 0.0f;
    }

    const i32 sampleCount = 2048;
    for (i32 i = 0; i < sampleCount; ++i)
    {
        const float4 dir = SampleDir(Hammersley2D(i, sampleCount), dist);
        const float sampleWeight = 1.0f / (i + 1.0f);

        for (i32 j = 0; j < count; ++j)
        {
            float cosTheta = f4_dot3(dir, sgs[j].axis);
            float weight = expf(sgs[j].sharpness * (cosTheta - 1.0f));
            float diff = weight * weight - sgs[j].basisSqIntegral;
            sgs[j].basisSqIntegral += diff * sampleWeight;
        }
    }
}
