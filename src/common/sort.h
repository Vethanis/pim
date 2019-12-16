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

    const i32 pivot = count >> 1;

    i32 i = 0;
    i32 j = count - 1;
    while (true)
    {
        while (items[i] < items[pivot])
        {
            ++i;
        }
        while (items[j] > items[pivot])
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

    const i32 pivot = count >> 1;

    i32 i = 0;
    i32 j = count - 1;
    while (true)
    {
        while (cmp(items[i], items[pivot]) < 0)
        {
            ++i;
        }
        while (cmp(items[j], items[pivot]) > 0)
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

    Sort(items, i, cmp);
    Sort(items + i, count - i, cmp);
}
