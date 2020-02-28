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
            while ((i < j) && (items[i] < pivot))
            {
                ++i;
            }
            while ((j > i) && (pivot < items[j]))
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

template<typename Container>
static void Sort(Container container)
{
    Sort(container.begin(), container.size());
}

template<typename T, typename C>
static void Sort(T* items, i32 count, C LtFn)
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
            while ((i < j) && LtFn(items[i], pivot))
            {
                ++i;
            }
            while ((j > i) && LtFn(pivot, items[j]))
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

    Sort(items, i, LtFn);
    Sort(items + i, count - i, LtFn);
}

template<typename T>
static i32 PushSort(T* const items, i32 countWithItem, const T& item)
{
    ASSERT(items);
    ASSERT(countWithItem > 0);

    const i32 back = countWithItem - 1;
    items[back] = item;

    i32 i = back;
    while ((i > 0) && (items[i] < items[i - 1]))
    {
        T tmp = items[i - 1];
        items[i - 1] = items[i];
        items[i] = tmp;
        --i;
    }

    ASSERT(i >= 0);
    if (i < back)
    {
        ASSERT(items[i] < items[i + 1]);
    }

    return i;
}
