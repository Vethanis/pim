#include "math/dist1d.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include "common/atomics.h"
#include <string.h>

void Dist1D_New(Dist1D *const dist, i32 length)
{
    memset(dist, 0, sizeof(*dist));
    if (length > 0)
    {
        i32 pdfLen = length;
        i32 cdfLen = length + 1;
        dist->length = length;
        dist->pdf = Perm_Calloc(sizeof(dist->pdf[0]) * pdfLen);
        dist->cdf = Perm_Calloc(sizeof(dist->cdf[0]) * cdfLen);
        dist->live = Perm_Calloc(sizeof(dist->live[0]) * pdfLen);
        dist->integral = 0.0f;
    }
}

void Dist1D_Del(Dist1D *const dist)
{
    if (dist)
    {
        Mem_Free(dist->pdf);
        Mem_Free(dist->cdf);
        Mem_Free(dist->live);
        memset(dist, 0, sizeof(*dist));
    }
}

void Dist1D_Bake(Dist1D *const dist)
{
    const i32 pdfLen = dist->length;
    const i32 cdfLen = pdfLen + 1;
    if (pdfLen > 0)
    {
        const float rcpLen = 1.0f / pdfLen;

        float *const pim_noalias pdf = dist->pdf;
        float *const pim_noalias cdf = dist->cdf;

        cdf[0] = 0.0f;
        for (i32 i = 1; i < cdfLen; ++i)
        {
            cdf[i] = cdf[i - 1] + pdf[i - 1] * rcpLen;
        }
        float integral = cdf[cdfLen - 1];

        if (integral == 0.0f)
        {
            for (i32 i = 1; i < cdfLen; ++i)
            {
                cdf[i] = i * rcpLen;
            }
        }
        else
        {
            float rcpIntegral = 1.0f / integral;
            for (i32 i = 1; i < cdfLen; ++i)
            {
                cdf[i] = cdf[i] * rcpIntegral;
            }
            for (i32 i = 0; i < pdfLen; ++i)
            {
                pdf[i] = pdf[i] * rcpIntegral;
            }
        }

        dist->integral = integral;
    }
}

pim_inline i32 FindInterval(float const *const pim_noalias cdf, i32 size, float u)
{
    i32 first = 0;
    i32 len = size;
    while (len > 0)
    {
        i32 half = len >> 1;
        i32 middle = first + half;
        if (cdf[middle] <= u)
        {
            first = middle + 1;
            len = len - (half + 1);
        }
        else
        {
            len = half;
        }
    }
    return i1_clamp(first - 1, 0, size - 2);
}

float Dist1D_SampleC(Dist1D const *const dist, float u)
{
    float const *const pim_noalias cdf = dist->cdf;
    const i32 pdfLen = dist->length;
    const i32 cdfLen = pdfLen + 1;
    i32 offset = FindInterval(cdf, cdfLen, u);
    float u0 = cdf[offset];
    float u1 = cdf[offset + 1];
    float w = u1 - u0;
    float du = u - u0;
    if (w > 0.0f)
    {
        du = du / w;
    }
    return (offset + du) / pdfLen;
}

i32 Dist1D_SampleD(Dist1D const *const dist, float u)
{
    return FindInterval(dist->cdf, dist->length + 1, u);
}

float Dist1D_PdfD(Dist1D const *const dist, i32 i)
{
    return dist->pdf[i] / dist->length;
}

void Dist1D_Inc(Dist1D *const dist, i32 i)
{
    inc_u32(dist->live + i, MO_Relaxed);
}

void Dist1D_Update(Dist1D *const dist)
{
    const i32 pdfLen = dist->length;
    if (pdfLen > 0)
    {
        float *const pim_noalias pdf = dist->pdf;
        u32 *const pim_noalias live = dist->live;
        u32 sum = 0;
        for (i32 i = 0; i < pdfLen; ++i)
        {
            sum += live[i];
        }
        if (sum < 30)
        {
            // less than 30 is a very very weak distribution
            return;
        }
        const float scale = 1.0f / sum;
        const u32 prevSum = dist->sum;
        dist->sum = sum;
        float alpha = 0.5f;
        if (prevSum > 0)
        {
            double ratio = (double)sum / (double)prevSum;
            alpha = f1_saturate((float)ratio) * 0.9f;
            alpha = alpha * alpha;
        }
        for (i32 i = 0; i < pdfLen; ++i)
        {
            u32 ct = live[i];
            pdf[i] = f1_lerp(pdf[i], ct * scale, alpha);
            live[i] = (ct >> 1); // retain some memory from last update
        }
        Dist1D_Bake(dist);
    }
}
