#include "math/markov_sampler.h"

#include "common/random.h"
#include "math/scalar.h"
#include "allocator/allocator.h"
#include <string.h>

SASSERT(sizeof(MarkovSampler) <= 48);

void MarkovSampler_New(
    MarkovSampler* s,
    u32 sampleCount)
{
    MarkovSample* pim_noalias samples = Perm_Alloc(sizeof(s->samples[0]) * sampleCount);
    Prng* rng = Prng_Get();
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float value = Prng_f32(rng);
        samples[i].value = value;
        samples[i].valueBackup = value;
        samples[i].modIter = 0;
        samples[i].modIterBackup = 0;
    }
    memset(s, 0, sizeof(*s));
    s->samples = samples;
    s->sampleCount = sampleCount;
    s->largeStep = 1;
}

void MarkovSampler_Del(MarkovSampler* s)
{
    Mem_Free(s->samples);
    memset(s, 0, sizeof(*s));
}

void MarkovSampler_Reset(MarkovSampler* s)
{
    s->iIteration = 0;
    s->iLastLargeIteration = 0;
    s->iSample = 0;
    s->largeStep = 1;
    u32 sampleCount = s->sampleCount;
    MarkovSample* pim_noalias samples = s->samples;
    Prng* rng = Prng_Get();
    for (u32 i = 0; i < sampleCount; ++i)
    {
        float value = Prng_f32(rng);
        samples[i].value = value;
        samples[i].valueBackup = value;
        samples[i].modIter = 0;
        samples[i].modIterBackup = 0;
    }
}

void MarkovSampler_StartIteration(MarkovSampler* s, float probLargeStep)
{
    Prng* rng = Prng_Get();
    s->iIteration++;
    s->iSample = 0;
    s->largeStep = Prng_f32(rng) < probLargeStep;
}

void MarkovSampler_Accept(MarkovSampler* s)
{
    if (s->largeStep)
    {
        s->iLastLargeIteration = s->iIteration;
    }
}

void MarkovSampler_Reject(MarkovSampler* s)
{
    u32 iIter = s->iIteration;
    u32 sampleCount = s->sampleCount;
    MarkovSample* pim_noalias samples = s->samples;
    for (u32 i = 0; i < sampleCount; ++i)
    {
        if (samples[i].modIter == iIter)
        {
            samples[i].value = samples[i].valueBackup;
            samples[i].modIter = samples[i].modIterBackup;
        }
    }
    s->iIteration--;
}

float VEC_CALL MarkovSampler_Sample1D(MarkovSampler* s, float sigma)
{
    float value = 0.0f;
    const u32 iIndex = s->iSample;
    s->iSample = iIndex + 1u;

    float4 Xv = Prng_float4(Prng_Get());
    if (iIndex < s->sampleCount)
    {
        MarkovSample* Xi = &s->samples[iIndex];
        if (Xi->modIter < s->iLastLargeIteration)
        {
            Xi->value = Xv.x;
            Xi->modIter = s->iLastLargeIteration;
        }

        Xi->valueBackup = Xi->value;
        Xi->modIterBackup = Xi->modIter;
        if (s->largeStep)
        {
            Xi->value = Xv.y;
        }
        else
        {
            u32 N = u1_max(s->iIteration - Xi->modIter, 1);
            float stddev = sigma * sqrtf((float)N);
            float z = f1_gauss_invcdf(Xv.z, 0.0f, stddev);
            z = (Xv.w < 0.5f) ? -z : z;
            Xi->value = f1_frac(Xi->value + z);
        }
        Xi->modIter = s->iIteration;

        value = Xi->value;
    }
    else
    {
        // ran out of space :(
        ASSERT(false);
        value = Xv.x;
    }

    return value;
}

float2 VEC_CALL MarkovSampler_Sample2D(MarkovSampler* s, float sigma)
{
    float2 v;
    v.x = MarkovSampler_Sample1D(s, sigma);
    v.y = MarkovSampler_Sample1D(s, sigma);
    return v;
}

float VEC_CALL Markov_AcceptProb(float proposedLum, float currentLum)
{
    return f1_sat(proposedLum / f1_max(currentLum, kEpsilon));
}

float VEC_CALL Markov_ProposedWeight(float acceptProb, float proposedLum)
{
    return (proposedLum > kEpsilon) ? (acceptProb / proposedLum) : 0.0f;
}

float VEC_CALL Markov_CurrentWeight(float acceptProb, float currentLum)
{
    return (currentLum > kEpsilon) ? ((1.0f - acceptProb) / currentLum) : 0.0f;
}
