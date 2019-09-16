#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/shift.h"

// ----------------------------------------------------------------------------
// string length

inline constexpr isize StrLen(cstrc x)
{
    isize i = 0;
    for(; x[i]; ++i)
    {}
    return i;
}

// ----------------------------------------------------------------------------
// case conversion

inline constexpr char ToLower(char c)
{
    constexpr char correction = 'a' - 'A';
    const char corrected = c + correction;
    return ((c >= 'A') & (c <= 'Z')) ? corrected : c;
}

inline constexpr char ToUpper(char c)
{
    constexpr char correction = 'A' - 'a';
    const char corrected = c + correction;
    return ((c >= 'a') & (c <= 'z')) ? corrected : c;
}

inline char* ToLower(char* dst)
{
    isize i = 0;
    for (; dst[i]; ++i)
    {
        dst[i] = ToLower(dst[i]);
    }
    return dst + i;
}

inline char* ToUpper(char* dst)
{
    isize i = 0;
    for (; dst[i]; ++i)
    {
        dst[i] = ToUpper(dst[i]);
    }
    return dst + i;
}

// ----------------------------------------------------------------------------
// string compare

inline constexpr i32 StrCmp(cstrc lhs, cstrc rhs)
{
    i32 cmp = 0;
    for (isize i = 0; (lhs[i] | rhs[i]) && !cmp; ++i)
    {
        cmp = lhs[i] - rhs[i];
    }
    return cmp;
}

inline constexpr i32 StrICmp(cstrc lhs, cstrc rhs)
{
    i32 cmp = 0;
    for (isize i = 0; (lhs[i] | rhs[i]) && !cmp; ++i)
    {
        cmp = ToLower(lhs[i]) - ToLower(rhs[i]);
    }
    return cmp;
}

// ----------------------------------------------------------------------------
// string searching

inline cstr StrChr(cstr hay, char needle)
{
    for (isize i = 0; hay[i]; ++i)
    {
        if (hay[i] == needle)
        {
            return hay + i;
        }
    }
    return 0;
}

inline cstr StrIChr(cstr hay, char needle)
{
    needle = ToLower(needle);
    for (isize i = 0; hay[i]; ++i)
    {
        if (ToLower(hay[i]) == needle)
        {
            return hay + i;
        }
    }
    return 0;
}

inline cstr StrRChr(cstr hay, char needle)
{
    cstr ptr = 0;
    for (isize i = 0; hay[i]; ++i)
    {
        ptr = (hay[i] == needle) ? hay : ptr;
    }
    return ptr;
}

inline cstr StrIRChr(cstr hay, char needle)
{
    cstr ptr = 0;
    needle = ToLower(needle);
    for (isize i = 0; hay[i]; ++i)
    {
        char c = ToLower(hay[i]);
        ptr = (c == needle) ? hay : ptr;
    }
    return ptr;
}

inline constexpr cstrc CStrStr(cstrc hay, cstrc needle)
{
    for (isize i = 0; hay[i]; ++i)
    {
        i32 cmp = 0;
        for (isize j = 0; needle[j] && !cmp; ++j)
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
inline cstr StrStr(cstr hay, cstr needle)
{
    return CStrStr(hay, needle);
}

inline constexpr cstrc CStrIStr(cstrc hay, cstrc needle)
{
    for (isize i = 0; hay[i]; ++i)
    {
        i32 cmp = 0;
        for (isize j = 0; needle[j] && !cmp; ++j)
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
inline cstr StrIStr(cstr hay, cstr needle)
{
    return CStrIStr(hay, needle);
}

// ----------------------------------------------------------------------------
// string formatting

char* VSPrintf(char* dst, isize size, cstr fmt, VaList va);

template<isize size>
inline char* VSPrintf(char(&dst)[size], cstr fmt, VaList va)
{
    return VSPrintf(dst, size, fmt, va);
}

inline char* SPrintf(char* dst, isize size, cstr fmt, ...)
{
    return VSPrintf(dst, size, fmt, VaStart(fmt));
}

template<isize size>
inline char* SPrintf(char(&dst)[size], cstr fmt, ...)
{
    return VSPrintf(dst, fmt, VaStart(fmt));
}

inline char* StrCatf(char* dst, isize size, cstr fmt, ...)
{
    dst[size - 1] = 0;
    const isize len = StrLen(dst);
    const isize rem = size - len;
    char* p = dst + len;
    return VSPrintf(p, rem, fmt, VaStart(fmt));
}

template<isize size>
inline char* StrCatf(char(&dst)[size], cstr fmt, ...)
{
    dst[size - 1] = 0;
    const isize len = StrLen(dst);
    const isize rem = size - len;
    char* p = dst + len;
    return VSPrintf(p, rem, fmt, VaStart(fmt));
}

// ----------------------------------------------------------------------------
// string copying

inline char* StrCpy(char* dst, isize size, cstr src)
{
    dst[size - 1] = 0;
    const isize len = size - 1;
    isize i;
    for (i = 0; i < len && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    dst[i] = 0;
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* StrCpy(char(&dst)[size], cstr src)
{
    return StrCpy(dst, size, src);
}

inline char* StrCat(char* dst, isize size, cstr src)
{
    dst[size - 1] = 0;
    const isize len = StrLen(dst);
    const isize rem = size - len;
    char* p = dst + len;
    return StrCpy(p, rem, src);
}
template<isize size>
inline char* StrCat(char(&dst)[size], cstr src)
{
    return StrCat(dst, size, src);
}

// ----------------------------------------------------------------------------
// char replace

inline char* FixPath(char* dst, isize size)
{
    dst[size - 1] = 0;
    isize i;
    for (i = 0; dst[i]; ++i)
    {
        char c = dst[i];
        dst[i] = c == '/' ? '\\' : c;
    }
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* FixPath(char(&dst)[size])
{
    return FixPath(dst, size);
}

inline char* ChrRep(char* dst, isize size, char fnd, char rep)
{
    dst[size - 1] = 0;
    isize i;
    for (i = 0; dst[i]; ++i)
    {
        char c = dst[i];
        dst[i] = c == fnd ? rep : c;
    }
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* ChrRep(char(&dst)[size], char fnd, char rep)
{
    return ChrRep(dst, size, fnd, rep);
}

inline char* ChrIRep(char* dst, isize size, char fnd, char rep)
{
    dst[size - 1] = 0;
    fnd = ToLower(fnd);
    isize i;
    for (i = 0; dst[i]; ++i)
    {
        char c = dst[i];
        dst[i] = ToLower(c) == fnd ? rep : c;
    }
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* ChrIRep(char(&dst)[size], char fnd, char rep)
{
    return ChrIRep(dst, size, fnd, rep);
}

// ----------------------------------------------------------------------------
// string replace

inline char* StrRep(char* dst, isize size, cstrc fnd, cstrc rep)
{
    dst[size - 1] = 0;
    const isize fndLen = StrLen(fnd);
    const isize repLen = StrLen(rep);
    const isize shifts = repLen - fndLen;
    isize dstLen = StrLen(dst);
    isize i = 0;
    while (1)
    {
        cstr p = StrStr(dst + i, fnd);
        if (!p) { break; }
        i = p - dst;
        dstLen = Shift(dst + i, dstLen, i, shifts);
        p = StrCpy(dst + i, size - i, rep);
        i = p - dst;
    }
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* StrRep(char(&dst)[size], cstrc fnd, cstrc rep)
{
    return StrRep(dst, size, fnd, rep);
}

inline char* StrIRep(char* dst, isize size, cstrc fnd, cstrc rep)
{
    dst[size - 1] = 0;
    const isize fndLen = StrLen(fnd);
    const isize repLen = StrLen(rep);
    const isize shifts = repLen - fndLen;
    isize dstLen = StrLen(dst);
    isize i = 0;
    while (1)
    {
        cstr p = StrIStr(dst + i, fnd);
        if (!p) { break; }
        i = p - dst;
        dstLen = Shift(dst + i, dstLen, i, shifts);
        p = StrCpy(dst + i, size - i, rep);
        i = p - dst;
    }
    dst[size - 1] = 0;
    return dst + i;
}
template<isize size>
inline char* StrIRep(char(&dst)[size], cstrc fnd, cstrc rep)
{
    return StrIRep(dst, size, fnd, rep);
}

