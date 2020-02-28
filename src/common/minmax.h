#pragma once

#include "common/int_types.h"

template<typename T>
inline constexpr T Min(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
inline constexpr T Max(T a, T b)
{
    return a > b ? a : b;
}

template<typename T>
inline constexpr T Clamp(T x, T lo, T hi)
{
    return Min(hi, Max(lo, x));
}

template<typename T>
inline constexpr T Align(T x, T align)
{
    const T mask = align - 1;
    T rem = x & mask;
    T fix = (align - rem) & mask;
    return x + fix;
}

template<typename T>
inline constexpr bool Adjacent(T lBegin, T lEnd, T rBegin, T rEnd)
{
    return (lBegin == rEnd) || (rBegin == lEnd);
}

template<typename T>
inline constexpr bool Overlaps(T lBegin, T lEnd, T rBegin, T rEnd)
{
    return (lBegin < rEnd) && (rBegin < lEnd);
}

template<typename C>
inline constexpr bool Adjacent(C lhs, C rhs)
{
    return Adjacent(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template<typename C>
inline constexpr bool Overlaps(C lhs, C rhs)
{
    return Overlaps(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static constexpr u32 ToPow2(u32 x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return x;
}

template<typename T>
static void Swap(T& lhs, T& rhs)
{
    T tmp = lhs;
    lhs = rhs;
    rhs = tmp;
}
