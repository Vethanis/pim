#pragma once

#include "common/macro.h"

typedef struct dist1d_s
{
    float* pim_noalias pdf;
    float* pim_noalias cdf;
    u32* pim_noalias live;
    i32 length;
    float integral;
} dist1d_t;

void dist1d_new(dist1d_t* dist, i32 length);
void dist1d_del(dist1d_t* dist);

void dist1d_bake(dist1d_t* dist);

// continuous
float dist1d_samplec(const dist1d_t* dist, float u);

// discrete
i32 dist1d_sampled(const dist1d_t* dist, float u);
float dist1d_pdfd(const dist1d_t* dist, i32 i);

void dist1d_inc(dist1d_t* dist, i32 i);
void dist1d_livebake(dist1d_t* dist, float alpha);
