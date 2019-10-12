#pragma once

#include "common/macro.h"

template<typename C, typename T>
inline void Apply(C container, T (*func)(T))
{
    for (T& item : container)
    {
        item = func(item);
    }
}

template<typename C, typename T>
inline void Apply(C container, void (*func)(T&))
{
    for (T& item : container)
    {
        func(item);
    }
}
