#pragma once

#include "common/macro.h"

typedef struct Dist1D_s
{
    float* pim_noalias pdf;
    float* pim_noalias cdf;
    u32* pim_noalias live;
    i32 length;
    float integral;
    u32 sum;
} Dist1D;

void Dist1D_New(Dist1D *const dist, i32 length);
void Dist1D_Del(Dist1D *const dist);

void Dist1D_Bake(Dist1D *const dist);

// continuous
float Dist1D_SampleC(Dist1D const *const dist, float u);

// discrete
i32 Dist1D_SampleD(Dist1D const *const dist, float u);
float Dist1D_PdfD(Dist1D const *const dist, i32 i);

void Dist1D_Inc(Dist1D *const dist, i32 i);
void Dist1D_Update(Dist1D *const dist);
