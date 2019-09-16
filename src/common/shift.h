#pragma once

#include "common/int_types.h"

// ----------------------------------------------------------------------------
// memory shifting

template<typename T>
inline isize ShiftRight(T* ptr, isize len, isize idx, isize shifts)
{
    for (isize i = len - 1; i > idx; --i)
    {
        ptr[i + shifts] = ptr[i];
    }
    return len + shifts;
}

template<typename T>
inline isize ShiftLeft(T* ptr, isize len, isize idx, isize shifts)
{
    for (isize i = idx; i < len; ++i)
    {
        ptr[i] = ptr[i + shifts];
    }
    return len - shifts;
}

template<typename T>
inline isize Shift(T* ptr, isize len, isize idx, isize shifts)
{
    return (shifts < 0) ?
        ShiftLeft(ptr, len, idx, -shifts) :
        ShiftRight(ptr, len, idx, shifts);
}

