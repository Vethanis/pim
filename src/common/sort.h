#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "allocator/allocator.h"
#include <string.h>

typedef i32(*CmpFn)(const void* lhs, const void* rhs, void* usr);

static void Swap(void* lhs, void* rhs, i32 stride)
{
    void* tmp = pim_pusha(stride);
    memcpy(tmp, lhs, stride);
    memcpy(lhs, rhs, stride);
    memcpy(rhs, tmp, stride);
    pim_popa(stride);
}

static void Sort(void* pVoid, i32 count, i32 stride, CmpFn cmp, void* usr)
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
                Swap(lhs, items + lo * stride, stride);
            }
        }
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        u8* pivot = pim_pusha(stride);
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

            Swap(items + i * stride, items + j * stride, stride);

            ++i;
            --j;
        }

        pim_popa(stride);
    }

    Sort(items, i, stride, cmp, usr);
    Sort(items + i * stride, count - i, stride, cmp, usr);
}

PIM_C_END
