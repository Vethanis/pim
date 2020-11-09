#pragma once

#include "common/macro.h"
#include <string.h>

template<typename T>
static void Swap(T& pim_noalias lhs, T& pim_noalias rhs)
{
    alignas(alignof(T)) u8 tmp[sizeof(T)];
    memcpy(&tmp, &lhs, sizeof(T));
    memcpy(&lhs, &rhs, sizeof(T));
    memcpy(&rhs, &tmp, sizeof(T));
}

template<typename T>
static void QuickSort(T *const pim_noalias ptr, i32 count)
{
    if (count <= 4)
    {
        for (i32 i = 0; i < count; ++i)
        {
            i32 lo = i;
            for (i32 j = i + 1; j < count; ++j)
            {
                if (ptr[j] < ptr[i])
                {
                    lo = j;
                }
            }
            if (lo != i)
            {
                Swap(ptr[i], ptr[lo]);
            }
        }
        return;
    }

    i32 i = 0;
    {
        alignas(alignof(T)) u8 tmp[sizeof(T)];
        memcpy(&tmp, ptr + (count >> 1), sizeof(T));
        const T& pim_noalias pivot = *(T*)&tmp;

        i32 j = count - 1;
        while (true)
        {
            while ((i < count) && (ptr[i] < pivot[0]))
            {
                ++i;
            }
            while ((j > 0) && (ptr[j] > pivot[0]))
            {
                --j;
            }
            if (i >= j)
            {
                break;
            }
            Swap(ptr[i], ptr[j]);
            ++i;
            --j;
        }
    }

    QuickSort(ptr, i);
    QuickSort(ptr + i, count - i);
}
