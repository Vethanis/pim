#pragma once

#include "common/macro.h"
#include "common/int_types.h"

// ----------------------------------------------------------------------------
// null termination

inline void NullTerminate(char* dst, i32 i, i32 size)
{
    DebugAssert(dst || !size);
    DebugAssert(i >= 0);
    DebugAssert(size >= 0);

    if (size > 0)
    {
        dst[Min(i, size - 1)] = 0;
    }
}

// ----------------------------------------------------------------------------
// character classes

inline constexpr bool IsUpper(char c)
{
    return (c >= 'A') && (c <= 'Z');
}

inline constexpr bool IsLower(char c)
{
    return (c >= 'a') && (c <= 'z');
}

inline constexpr bool IsAlpha(char c)
{
    return IsLower(c) || IsUpper(c);
}

inline constexpr bool IsNumeric(char c)
{
    return (c >= '0') && (c <= '9');
}

inline constexpr bool IsAlphaNumeric(char c)
{
    return IsAlpha(c) || IsNumeric(c);
}

inline constexpr bool IsVarChar0(char c)
{
    return IsAlpha(c) || (c == '_');
}

inline constexpr bool IsVarChar1(char c)
{
    return IsVarChar0(c) || IsNumeric(c);
}

inline constexpr bool IsVarName(cstrc x)
{
    DebugAssert(x);
    if (!IsVarChar0(x[0]))
    {
        return false;
    }
    for (i32 i = 1; x[i]; ++i)
    {
        if (!IsVarChar1(x[i]))
        {
            return false;
        }
    }
    return true;
}

inline bool IsWhitespace(char c)
{
    switch (c)
    {
    case '\0':
    case ' ':
    case '\n':
    case '\r':
    case '\t':
        return true;
    default:
        return false;
    }
}

// ----------------------------------------------------------------------------
// case conversion

inline constexpr char ToLower(char c)
{
    constexpr char correction = 'a' - 'A';
    const char corrected = c + correction;
    return IsUpper(c) ? corrected : c;
}

inline constexpr char ToUpper(char c)
{
    constexpr char correction = 'A' - 'a';
    const char corrected = c + correction;
    return IsLower(c) ? corrected : c;
}

inline void ToLower(char* dst, i32 size)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for(; i < size && dst[i]; ++i)
    {
        dst[i] = ToLower(dst[i]);
    }
    NullTerminate(dst, i, size);
}

inline char* ToUpper(char* dst, i32 size)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ToUpper(dst[i]);
    }
    NullTerminate(dst, i, size);
}

// ----------------------------------------------------------------------------
// string length

inline constexpr i32 StrLen(cstrc x)
{
    i32 i = 0;
    if (x)
    {
        while (x[i])
        {
            ++i;
        }
    }
    return i;
}

inline constexpr i32 StrLen(cstrc x, i32 size)
{
    DebugAssert(x || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    if (x)
    {
        while (i < size && x[i])
        {
            ++i;
        }
    }
    return i;
}

// ----------------------------------------------------------------------------
// string copying

inline void StrCpy(char* dst, i32 size, cstrc src)
{
    DebugAssert((dst && src) || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for (; i < size && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    NullTerminate(dst, i, size);
}

inline void StrCat(char* dst, i32 size, cstrc src)
{
    DebugAssert((dst && src) || !size);
    DebugAssert(size >= 0);

    i32 len = StrLen(dst, size);
    StrCpy(dst + len, size - len, src);
}

// ----------------------------------------------------------------------------
// string compare

inline constexpr i32 StrCmp(cstrc lhs, i32 size, cstrc rhs)
{
    DebugAssert((lhs && rhs) || !size);
    DebugAssert(size >= 0);

    i32 c = 0;
    for (i32 i = 0; !c && i < size; ++i)
    {
        c = lhs[i] - rhs[i];
        if (!lhs[i])
        {
            break;
        }
    }
    return c;
}

inline constexpr i32 StrICmp(cstrc lhs, i32 size, cstrc rhs)
{
    DebugAssert((lhs && rhs) || !size);
    DebugAssert(size >= 0);

    i32 c = 0;
    for (i32 i = 0; !c && i < size; ++i)
    {
        c = ToLower(lhs[i]) - ToLower(rhs[i]);
        if (!lhs[i])
        {
            break;
        }
    }
    return c;
}

// ----------------------------------------------------------------------------
// string searching

inline cstr StrChr(cstr hay, i32 size, char needle)
{
    DebugAssert(hay || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for (; i < size && hay[i]; ++i)
    {
        if (hay[i] == needle)
        {
            return hay + i;
        }
    }
    return 0;
}

inline cstr StrIChr(cstr hay, i32 size, char needle)
{
    DebugAssert(hay || !size);
    DebugAssert(size >= 0);

    needle = ToLower(needle);
    i32 i = 0;
    for (; i < size && hay[i]; ++i)
    {
        if (ToLower(hay[i]) == needle)
        {
            return hay + i;
        }
    }
    return 0;
}

inline cstr StrRChr(cstr hay, i32 size, char needle)
{
    DebugAssert(hay || !size);
    DebugAssert(size >= 0);

    cstr p = 0;
    i32 i = 0;
    for (; i < size && hay[i]; ++i)
    {
        if (hay[i] == needle)
        {
            p = hay + i;
        }
    }
    return p;
}

inline cstr StrIRChr(cstr hay, i32 size, char needle)
{
    DebugAssert(hay || !size);
    DebugAssert(size >= 0);

    needle = ToLower(needle);
    cstr p = 0;
    i32 i = 0;
    for (; i < size && hay[i]; ++i)
    {
        if (ToLower(hay[i]) == needle)
        {
            p = hay + i;
        }
    }
    return p;
}

inline cstr StrStr(cstr hay, i32 size, cstrc needle)
{
    DebugAssert((hay && needle) || !size);
    DebugAssert(size >= 0);

    const i32 hayLen = StrLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 outerLen = hayLen - needleLen;
    for (i32 i = 0; i <= outerLen; ++i)
    {
        i32 cmp = 0;
        for (i32 j = 0; j < needleLen && !cmp; ++j)
        {
            cmp = hay[i + j] - needle[j];
        }
        if (!cmp)
        {
            return hay + i;
        }
    }
    return 0;
}

inline cstr StrIStr(cstr hay, i32 size, cstrc needle)
{
    DebugAssert((hay && needle) || !size);
    DebugAssert(size >= 0);

    const i32 hayLen = StrLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 outerLen = hayLen - needleLen;
    for(i32 i = 0; i <= outerLen; ++i)
    {
        i32 cmp = 0;
        for (i32 j = 0; j < needleLen && !cmp; ++j)
        {
            cmp = ToLower(hay[i + j]) - ToLower(needle[j]);
        }
        if (!cmp)
        {
            return hay + i;
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// paths

inline char FixPath(char c)
{
    switch (c)
    {
    IfWin32(case '/': return '\\';)
    IfUnix(case '\\': return '/';)
    default: return c;
    };
}

inline void FixPath(char* dst, i32 size)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for(; i < size && dst[i]; ++i)
    {
        dst[i] = FixPath(dst[i]);
    }
    NullTerminate(dst, i, size);
}

inline void PathCpy(char* dst, i32 size, cstrc src)
{
    DebugAssert((dst && src) || !size);
    DebugAssert(size >= 0);

    StrCpy(dst, size, src);
    FixPath(dst, size);
}

inline void PathCat(char* dst, i32 size, cstr src)
{
    DebugAssert((dst && src) || !size);
    DebugAssert(size >= 0);

    i32 len = StrLen(dst, size);
    PathCpy(dst + len, size - len, src);
}

// ----------------------------------------------------------------------------
// string formatting

void VSPrintf(char* dst, i32 size, cstr fmt, VaList va);

inline void SPrintf(char* dst, i32 size, cstr fmt, ...)
{
    VSPrintf(dst, size, fmt, VaStart(fmt));
}

inline void StrCatf(char* dst, i32 size, cstr fmt, ...)
{
    i32 len = StrLen(dst, size);
    VSPrintf(dst + len, size - len, fmt, VaStart(fmt));
}

// ----------------------------------------------------------------------------
// char replace

inline void ChrRep(char* dst, i32 size, char fnd, char rep)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (dst[i] == fnd) ? rep : dst[i];
    }
    NullTerminate(dst, i, size);
}

inline void ChrIRep(char* dst, i32 size, char fnd, char rep)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    fnd = ToLower(fnd);
    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (ToLower(dst[i]) == fnd) ? rep : dst[i];
    }
    NullTerminate(dst, i, size);
}

// ----------------------------------------------------------------------------
// string replace

inline void ShiftRight(char* dst, i32 size, i32 shifts)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 len = StrLen(dst, size);
    len = Min(size - shifts - 1, len);
    for (i32 i = len; i >= 0; --i)
    {
        dst[i + shifts] = dst[i];
    }
    dst[size - 1] = 0;
}

inline void ShiftLeft(char* dst, i32 size, i32 shifts)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    i32 len = StrLen(dst, size);
    i32 i = shifts;
    for (; i < len; ++i)
    {
        dst[i - shifts] = dst[i];
    }
    NullTerminate(dst, i, size);
}

inline void Shift(char* dst, i32 size, i32 shifts)
{
    DebugAssert(dst || !size);
    DebugAssert(size >= 0);

    return (shifts < 0) ?
        ShiftLeft(dst, size, -shifts) :
        ShiftRight(dst, size, shifts);
}

inline i32 StrRep(char* dst, i32 size, cstr fnd, cstr rep)
{
    DebugAssert((dst && fnd && rep) || !size);
    DebugAssert(size >= 0);

    i32 ct = 0;
    i32 last = 0;
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);
    const i32 lenDiff = repLen - fndLen;

    char* inst = (char*)StrStr(dst, size, fnd);
    while (inst)
    {
        ++ct;
        const i32 i = (i32)(inst - dst);
        const i32 instSize = size - i;
        Shift(inst, instSize, lenDiff);
        const i32 cpyLen = Min(instSize, repLen);
        for (i32 j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }
        last = Max(last, i + cpyLen);
        inst = (char*)StrStr(inst, instSize, fnd);
    }
    NullTerminate(dst, last, size);

    return ct;
}

inline i32 StrIRep(char* dst, i32 size, cstr fnd, cstr rep)
{
    DebugAssert((dst && fnd && rep) || !size);
    DebugAssert(size >= 0);

    i32 ct = 0;
    i32 last = 0;
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);
    const i32 lenDiff = repLen - fndLen;

    char* inst = (char*)StrIStr(dst, size, fnd);
    while (inst)
    {
        ++ct;
        const i32 i = (i32)(inst - dst);
        const i32 instSize = size - i;
        Shift(inst, instSize, lenDiff);
        const i32 cpyLen = Min(instSize, repLen);
        for (i32 j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }
        last = Max(last, i + cpyLen);
        inst = (char*)StrIStr(inst, instSize, fnd);
    }
    NullTerminate(dst, last, size);

    return ct;
}

