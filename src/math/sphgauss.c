#include "math/sphgauss.h"
#include "allocator/allocator.h"
#include "math/sampling.h"
#include "common/random.h"

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
    i32 iSample,
    float4 dir,
    float4 rad,
    const float4* axii,
    float4* amplitudes,
    float* lobeWeights,
    const float* basisSqIntegrals,
    i32 length)
{
    if (iSample == 0)
    {
        for (i32 i = 0; i < length; ++i)
        {
            lobeWeights[i] = 0.0f;
        }
        for (i32 i = 0; i < length; ++i)
        {
            amplitudes[i] = f4_0;
        }
    }

    float* sampleWeights = tmp_malloc(sizeof(sampleWeights[0]) * length);

    float4 est = f4_0;
    for (i32 i = 0; i < length; ++i)
    {
        float w = SG_BasisEval(axii[i], dir);
        est = f4_add(est, f4_mulvs(amplitudes[i], w));
        sampleWeights[i] = w;
    }

    const float sampleScale = 1.0f / (iSample + 1.0f);
    for (i32 i = 0; i < length; ++i)
    {
        float sampleWeight = sampleWeights[i];
        if (sampleWeight == 0.0f)
        {
            continue;
        }

        lobeWeights[i] += ((sampleWeight * sampleWeight) - lobeWeights[i]) * sampleScale;
        float integral = f1_max(lobeWeights[i], basisSqIntegrals[i]);
        float4 otherLight = f4_sub(est, f4_mulvs(amplitudes[i], sampleWeight));
        float4 newEst = f4_mulvs(f4_sub(rad, otherLight), sampleWeight / integral);
        amplitudes[i] = f4_max(f4_lerpvs(amplitudes[i], newEst, sampleScale), f4_0);
    }
}

void SG_Generate(float4* axii, float* basisSqIntegrals, i32 count, SGDist dist)
{
    ASSERT(axii);
    ASSERT(basisSqIntegrals);
    ASSERT(count >= 0);

    for (i32 i = 0; i < count; ++i)
    {
        axii[i] = SampleDir(Hammersley2D(i, count), dist);
    }

    // shuffle directions
    prng_t rng = prng_get();
    for (i32 i = 0; i < count; ++i)
    {
        i32 j = prng_i32(&rng) % count;
        ASSERT(j >= 0);
        ASSERT(j < count);
        float4 tmp = axii[i];
        axii[i] = axii[j];
        axii[j] = tmp;
    }
    prng_set(rng);

    // find cosine of the smallest half-angle between axii
    float minCosTheta = 1.0f;
    const float4 axii0 = axii[0];
    for (i32 i = 1; i < count; ++i)
    {
        float4 H = f4_normalize3(f4_add(axii[i], axii0));
        minCosTheta = f1_min(minCosTheta, f1_sat(f4_dot3(H, axii0)));
    }

    float sharpness = (logf(0.65f) * count) / (minCosTheta - 1.0f);
    for (i32 i = 0; i < count; ++i)
    {
        axii[i].w = sharpness;
        basisSqIntegrals[i] = 0.0f;
    }

    const i32 sampleCount = 4096;
    const float sampleWeight = 1.0f / sampleCount;
    for (i32 i = 0; i < sampleCount; ++i)
    {
        float4 dir = SampleDir(Hammersley2D(i, sampleCount), dist);
        for (i32 j = 0; j < count; ++j)
        {
            float basis = SG_BasisEval(axii[j], dir);
            basisSqIntegrals[j] = f1_lerp(basisSqIntegrals[j], basis * basis, sampleWeight);
        }
    }

    // I think the spherical case has a closed form?
    // Worth testing with an assert.
    // if it passes, use it instead of sampling directions and integrating
    if (dist == SGDist_Sphere)
    {
        for (i32 i = 0; i < count; ++i)
        {
            float closedForm = SG_BasisIntegralSq(axii[i].w);
            float analytic = basisSqIntegrals[i];
            float dist = f1_distance(analytic, closedForm);
            float threshold = 0.01f * analytic;
            ASSERT(dist < threshold);
        }
    }
}
