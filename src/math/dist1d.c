#include "math/dist1d.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include "common/atomics.h"
#include <string.h>

void dist1d_new(dist1d_t* dist, i32 length)
{
    memset(dist, 0, sizeof(*dist));
    if (length > 0)
    {
        i32 pdfLen = length;
        i32 cdfLen = length + 1;
        dist->length = length;
        dist->pdf = perm_calloc(sizeof(dist->pdf[0]) * pdfLen);
        dist->cdf = perm_calloc(sizeof(dist->cdf[0]) * cdfLen);
        dist->live = perm_calloc(sizeof(dist->live[0]) * pdfLen);
        dist->integral = 0.0f;
    }
}

void dist1d_del(dist1d_t* dist)
{
    if (dist)
    {
        pim_free(dist->pdf);
        pim_free(dist->cdf);
        pim_free(dist->live);
        memset(dist, 0, sizeof(*dist));
    }
}

void dist1d_bake(dist1d_t* dist)
{
    const i32 pdfLen = dist->length;
    const i32 cdfLen = pdfLen + 1;
    if (pdfLen > 0)
    {
        const float rcpLen = 1.0f / pdfLen;

        float* pim_noalias pdf = dist->pdf;
        float* pim_noalias cdf = dist->cdf;

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

pim_inline i32 FindInterval(const float* cdf, i32 size, float u)
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

float dist1d_samplec(const dist1d_t* dist, float u)
{
    const float* cdf = dist->cdf;
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

i32 dist1d_sampled(const dist1d_t* dist, float u)
{
    return FindInterval(dist->cdf, dist->length + 1, u);
}

float dist1d_pdfd(const dist1d_t* dist, i32 i)
{
    return dist->pdf[i] / dist->length;
}

void dist1d_inc(dist1d_t* dist, i32 i)
{
    inc_u32(dist->live + i, MO_Relaxed);
}

void dist1d_livebake(dist1d_t* dist, float alpha)
{
    const i32 pdfLen = dist->length;
    if (pdfLen > 0)
    {
        float* pim_noalias pdf = dist->pdf;
        u32* pim_noalias live = dist->live;
        u32 sum = 0;
        for (i32 i = 0; i < pdfLen; ++i)
        {
            u32 ct = live[i];
            sum += ct;
        }
        if (sum < 30)
        {
            return;
        }
        float scale = 1.0f / sum;
        for (i32 i = 0; i < pdfLen; ++i)
        {
            pdf[i] = f1_lerp(pdf[i], live[i] * scale, alpha);
            live[i] = live[i] >> 1;
        }
        dist1d_bake(dist);
    }
}
