#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "allocator/allocator.h"
#include "common/pimcpy.h"

typedef i32(*CmpFn)(const void* lhs, const void* rhs, void* usr);

static void pimswap(void* pim_noalias lhs, void* pim_noalias rhs, i32 stride)
{
    void* pim_noalias tmp = pim_pusha(stride);
    pimcpy(tmp, lhs, stride);
    pimcpy(lhs, rhs, stride);
    pimcpy(rhs, tmp, stride);
    pim_popa(stride);
}

static void pimsort(void* pVoid, i32 count, i32 stride, CmpFn cmp, void* usr)
{
    ASSERT(pVoid || !count);
    ASSERT(stride > 0);
    ASSERT(count >= 0);
    ASSERT(cmp);

    u8* items = pVoid;

    if (count <= 4)
    {
        for (i32 i = 0; i < count; ++i)
        {
            void* lhs = items + i * stride;
            i32 lo = i;
            for (i32 j = i + 1; j < count; ++j)
            {
                if (cmp(items + j * stride, items + lo * stride, usr) < 0)
                {
                    lo = j;
                }
            }
            if (lo != i)
            {
                pimswap(lhs, items + lo * stride, stride);
            }
        }
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        void* pim_noalias pivot = pim_pusha(stride);
        pimcpy(pivot, items + stride * (count >> 1), stride);

        while (true)
        {
            while ((i < count) && (cmp(items + i * stride, pivot, usr) < 0))
            {
                ++i;
            }
            while ((j > 0) && (cmp(items + j * stride, pivot, usr) > 0))
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            pimswap(items + i * stride, items + j * stride, stride);

            ++i;
            --j;
        }

        pim_popa(stride);
    }

    pimsort(items, i, stride, cmp, usr);
    pimsort(items + i * stride, count - i, stride, cmp, usr);
}

typedef struct indsort_ctx_s
{
    const u8* items;
    i32 stride;
    CmpFn cmp;
    void* usr;
} indsort_ctx_t;

static i32 indcmp(
    const void* pim_noalias lhs,
    const void* pim_noalias rhs,
    void* pim_noalias usr)
{
    const i32 a = *(i32*)lhs;
    const i32 b = *(i32*)rhs;
    const indsort_ctx_t* ctx = usr;
    return ctx->cmp(
        ctx->items + a * ctx->stride,
        ctx->items + b * ctx->stride,
        ctx->usr);
}

static i32* indsort(const void* items, i32 count, i32 stride, CmpFn cmp, void* usr)
{
    ASSERT(!count || items);
    ASSERT(stride);
    ASSERT(cmp);

    i32* indices = pim_malloc(EAlloc_Temp, sizeof(indices[0]) * count);
    for (i32 i = 0; i < count; ++i)
    {
        indices[i] = i;
    }

    indsort_ctx_t ctx;
    ctx.items = items;
    ctx.stride = stride;
    ctx.cmp = cmp;
    ctx.usr = usr;

    pimsort(indices, count, sizeof(indices[0]), indcmp, &ctx);
    return indices;
}

PIM_C_END
