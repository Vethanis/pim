#include "common/sort.h"
#include "allocator/allocator.h"
#include <string.h>

void MemSwap(void* lhs, void* rhs, i32 stride)
{
    void* tmp = Mem_Push(stride);
    memcpy(tmp, lhs, stride);
    memcpy(lhs, rhs, stride);
    memcpy(rhs, tmp, stride);
    Mem_Pop(stride);
}

void QuickSort(void* pVoid, i32 count, i32 stride, CmpFn cmp, void* usr)
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
                MemSwap(lhs, items + lo * stride, stride);
            }
        }
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        void* pivot = Mem_Push(stride);
        memcpy(pivot, items + stride * (count >> 1), stride);

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

            MemSwap(items + i * stride, items + j * stride, stride);

            ++i;
            --j;
        }

        Mem_Pop(stride);
    }

    QuickSort(items, i, stride, cmp, usr);
    QuickSort(items + i * stride, count - i, stride, cmp, usr);
}

void QuickSort_Int(i32* items, i32 count, CmpFn_i32 cmp, void* usr)
{
    ASSERT(items || !count);
    ASSERT(count >= 0);
    ASSERT(cmp);

    if (count <= 4)
    {
        for (i32 i = 0; i < count; ++i)
        {
            i32 lo = i;
            for (i32 j = i + 1; j < count; ++j)
            {
                if (cmp(items[j], items[lo], usr) < 0)
                {
                    lo = j;
                }
            }
            if (lo != i)
            {
                i32 tmp = items[i];
                items[i] = items[lo];
                items[lo] = tmp;
            }
        }
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        const i32 pivot = items[count >> 1];

        while (true)
        {
            while ((i < count) && (cmp(items[i], pivot, usr) < 0))
            {
                ++i;
            }
            while ((j > 0) && (cmp(items[j], pivot, usr) > 0))
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            i32 tmp = items[i];
            items[i] = items[j];
            items[j] = tmp;

            ++i;
            --j;
        }
    }

    QuickSort_Int(items, i, cmp, usr);
    QuickSort_Int(items + i, count - i, cmp, usr);
}

typedef struct indsort_ctx_s
{
    const u8* items;
    i32 stride;
    CmpFn cmp;
    void* usr;
} indsort_ctx_t;

static i32 indcmp(
    const i32 lhs,
    const i32 rhs,
    void* usr)
{
    const indsort_ctx_t* ctx = usr;
    return ctx->cmp(
        ctx->items + lhs * ctx->stride,
        ctx->items + rhs * ctx->stride,
        ctx->usr);
}

i32* IndexSort(const void* items, i32 count, i32 stride, CmpFn cmp, void* usr)
{
    ASSERT(!count || items);
    ASSERT(stride);
    ASSERT(cmp);

    i32* indices = Temp_Alloc(sizeof(indices[0]) * count);
    for (i32 i = 0; i < count; ++i)
    {
        indices[i] = i;
    }

    indsort_ctx_t ctx;
    ctx.items = items;
    ctx.stride = stride;
    ctx.cmp = cmp;
    ctx.usr = usr;

    QuickSort_Int(indices, count, indcmp, &ctx);
    return indices;
}
