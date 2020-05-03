#include "math/sphgauss.h"
#include "allocator/allocator.h"

void SGAccumulate(
    i32 iSample,
    float3 sampleDir,
    float3 sampleLight,
    SG_t* sgs,
    float* weights,
    i32 length)
{
    const float scale = 1.0f / (iSample + 1.0f);
    const i32 pushBytes = sizeof(float) * length;
    float* tmpWeights = pim_pusha(pushBytes);

    float3 est = f3_0;
    for (i32 i = 0; i < length; ++i)
    {
        const SG_t sg = sgs[i];
        float d = f3_dot(sg.axis, sampleDir);
        float w = expf(sg.sharpness * (d - 1.0f));
        est = f3_add(est, f3_mulvs(sg.amplitude, w));
        tmpWeights[i] = w;
    }

    for (i32 i = 0; i < length; ++i)
    {
        float w = tmpWeights[i];
        if (w == 0.0f)
        {
            continue;
        }
        SG_t sg = sgs[i];
        float weight = weights[i];

        weight += ((w*w) - weight) * scale;
        float integral = f1_max(weight, SGBasisIntegral(sg));
        float3 otherLight = f3_sub(est, f3_mulvs(sg.amplitude, w));
        float3 newEst = f3_mulvs(f3_sub(sampleLight, otherLight), w / integral);
        float3 diff = f3_mulvs(f3_sub(newEst, sg.amplitude), scale);
        sg.amplitude = f3_add(sg.amplitude, diff);
        // non-negative fit
        sg.amplitude = f3_max(sg.amplitude, f3_0);

        sgs[i] = sg;
        weights[i] = weight;
    }

    pim_popa(pushBytes);
}
