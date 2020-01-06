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
static i32 PushSort(T* items, i32 countWithItem, T item, const Comparable<T> cmp)
{
    ASSERT(items);
    ASSERT(countWithItem > 0);

    const i32 back = countWithItem - 1;
    items[back] = item;

    i32 pos = back;
    for (i32 i = back; i > 0; --i)
    {
        const i32 lhs = i - 1;
        if (cmp(items[lhs], items[i]) > 0)
        {
            T tmp = items[lhs];
            items[lhs] = items[i];
            items[i] = tmp;
            pos = lhs;
        }
        else
        {
            break;
        }
    }

    ASSERT(pos >= 0);
    ASSERT(cmp(items[pos], item) == 0);

    return pos;
}
