#include "math/sphgauss.h"
#include "allocator/allocator.h"

void SGAccumulate(
    i32 iSample,
    float3 sampleDir,
    float3 sampleLight,
    SG_t* sgs,
    float* lobeWeights,
    i32 length)
{
    const float sampleScale = 1.0f / (iSample + 1.0f);
    const i32 pushBytes = sizeof(float) * length;
    float* sampleWeights = pim_pusha(pushBytes);

    float3 est = f3_0;
    for (i32 i = 0; i < length; ++i)
    {
        const SG_t sg = sgs[i];
        float cosTheta = f3_dot(sg.axis, sampleDir);
        float sampleWeight = expf(sg.sharpness * (cosTheta - 1.0f));
        est = f3_add(est, f3_mulvs(sg.amplitude, sampleWeight));
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
        float lobeWeight = lobeWeights[i];

        lobeWeight += ((sampleWeight * sampleWeight) - lobeWeight) * sampleScale;
        float integral = f1_max(lobeWeight, SGBasisIntegralSq(sg));
        float3 otherLight = f3_sub(est, f3_mulvs(sg.amplitude, sampleWeight));
        float3 newEst = f3_mulvs(
            f3_sub(sampleLight, otherLight),
            sampleWeight / integral);
        float3 diff = f3_mulvs(f3_sub(newEst, sg.amplitude), sampleScale);
        sg.amplitude = f3_add(sg.amplitude, diff);
        // non-negative fit
        sg.amplitude = f3_max(sg.amplitude, f3_0);

        sgs[i] = sg;
        lobeWeights[i] = lobeWeight;
    }

    pim_popa(pushBytes);
}
