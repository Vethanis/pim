#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
static void Sort(T* items, i32 count)
{
    if (count < 2)
    {
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        const T pivot = items[count >> 1];
        while (true)
        {
            while (items[i] < pivot)
            {
                ++i;
            }
            while (items[j] > pivot)
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            T temp = items[i];
            items[i] = items[j];
            items[j] = temp;

            ++i;
            --j;
        }
    }

    Sort(items, i);
    Sort(items + i, count - i);
}

template<typename T, typename C>
static void Sort(T* items, i32 count, C cmp)
{
    if (count < 2)
    {
        return;
    }

    i32 i = 0;
    {
        i32 j = count - 1;
        const T pivot = items[count >> 1];
        while (true)
        {
            while (cmp(items[i], pivot) < 0)
            {
                ++i;
            }
            while (cmp(items[j], pivot) > 0)
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            T temp = items[i];
            items[i] = items[j];
            items[j] = temp;

            ++i;
            --j;
        }
    }

    Sort(items, i, cmp);
    Sort(items + i, count - i, cmp);
}
