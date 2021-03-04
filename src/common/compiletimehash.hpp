#pragma once

#include "common/macro.h"

inline constexpr char CompileTimeUpper(char x)
{
    i32 c = (i32)x;
    i32 lo = ('a' - 1) - c;     // c >= 'a'
    i32 hi = c - ('z' + 1);     // c <= 'z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

inline constexpr u32 CompileTimeHash32(char const *const str)
{
    u32 hash = 0;
    if (str && str[0])
    {
        hash = (u32)2166136261u;
        for (i32 i = 0; str[i]; ++i)
        {
            char c = CompileTimeUpper(str[i]);
            hash = (hash ^ c) * (u32)16777619u;
        }
        hash = hash ? hash : 1;
    }
    return hash;
}
