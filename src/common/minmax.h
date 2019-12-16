#pragma once

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
