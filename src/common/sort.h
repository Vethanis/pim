#pragma once

#include "common/macro.h"
#include "common/int_types.h"

template<typename T>
static void Sort(T* const items, i32 count)
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
            while (pivot < items[j])
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            {
                T temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }

            ++i;
            --j;
        }
    }

    Sort(items, i);
    Sort(items + i, count - i);
}

template<typename T, typename Lt>
static void Sort(T* const items, i32 count, const Lt& lt)
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
            while (lt(items[i], pivot))
            {
                ++i;
            }
            while (lt(pivot, items[j]))
            {
                --j;
            }

            if (i >= j)
            {
                break;
            }

            {
                T temp = items[i];
                items[i] = items[j];
                items[j] = temp;
            }

            ++i;
            --j;
        }
    }

    Sort(items, i, lt);
    Sort(items + i, count - i, lt);
}
