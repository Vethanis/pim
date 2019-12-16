#pragma once

// Divide x by y while rounding up
template<typename T>
inline constexpr T DivRound(T x, T y)
{
    return (x + y - 1) / y;
}

// Scale x to a multiple of y
template<typename T>
inline constexpr T MultipleOf(T x, T y)
{
    return DivRound(x, y) * y;
}

// Returns the remainder of x when divided by y
// Assumes that y is a power of 2
template<typename T>
inline constexpr T AlignRem(T x, T y)
{
    return (x + y - 1) & (~(y - 1));
}

// Returns the remainder of x when divided by the address size
template<typename T>
inline constexpr T AlignRemPtr(T x)
{
    return AlignRem(x, sizeof(void*));
}
