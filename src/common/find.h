#pragma once

#include "common/macro.h"

template<typename C, typename T>
inline const T* Find(const C& container, const T& key)
{
    for (const T& item : container)
    {
        if (item == key)
        {
            return &item;
        }
    }
    return nullptr;
}
