#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/comparator.h"

template<typename T>
static void Sort(T* items, i32 count, const Comparable<T> cmp)
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
            while ((i < j) && (cmp(items[i], pivot) < 0))
            {
                ++i;
            }
            while ((j > i) && (cmp(items[j], pivot) > 0))
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

    Sort(items, i, cmp);
    Sort(items + i, count - i, cmp);
}

template<typename T>
static void AddSort(T* items, i32 countWithItem, T item, const Comparable<T> cmp)
{
    const i32 back = countWithItem - 1;
    items[back] = item;

    for (i32 i = back; i > 0; --i)
    {
        if (cmp(items[i - 1], items[i]) > 0)
        {
            T tmp = items[i - 1];
            items[i - 1] = items[i];
            items[i] = tmp;
        }
        else
        {
            break;
        }
    }
}
