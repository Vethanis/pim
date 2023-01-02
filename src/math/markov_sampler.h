#pragma once

#include "math/types.h"

PIM_C_BEGIN

void MarkovSampler_New(
    MarkovSampler* s,
    u32 sampleCount);
void MarkovSampler_Del(MarkovSampler* s);

void MarkovSampler_Reset(MarkovSampler* s);

void MarkovSampler_StartIteration(MarkovSampler* s, float probLargeStep);

void MarkovSampler_Accept(MarkovSampler* s);
void MarkovSampler_Reject(MarkovSampler* s);

float VEC_CALL MarkovSampler_Sample1D(MarkovSampler* s, float sigma);
float2 VEC_CALL MarkovSampler_Sample2D(MarkovSampler* s, float sigma);

float VEC_CALL Markov_AcceptProb(float proposedLum, float currentLum);
float VEC_CALL Markov_ProposedWeight(float acceptProb, float proposedLum);
float VEC_CALL Markov_CurrentWeight(float acceptProb, float currentLum);

PIM_C_END
