#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/tolower.h"

inline void ToLower(char* const dst)
{
    if (dst)
    {
        for (usize i = 0; dst[i]; ++i)
        {
            dst[i] = ToLower(dst[i]);
        }
    }
}

inline usize StrLen(cstr x)
{
    usize i = 0;
    if (x)
    {
        while (x[i])
        {
            ++i;
        }
    }
    return i;
}

inline cstr StrBack(cstr x)
{
    if (x)
    {
        usize len = StrLen(x);
        return x + (len ? len - 1 : 0);
    }
    return 0;
}

inline cstr StrChr(cstr hay, char needle)
{
    if (hay)
    {
        for (isize i = 0; hay[i]; ++i)
        {
            if (hay[i] == needle)
            {
                return hay + i;
            }
        }
    }
    return 0;
}

inline cstr StrIChr(cstr hay, char needle)
{
    if (hay)
    {
        needle = ToLower(needle);
        for (isize i = 0; hay[i]; ++i)
        {
            if (ToLower(hay[i]) == needle)
            {
                return hay + i;
            }
        }
    }
    return 0;
}

inline cstr StrRChr(cstr hay, char needle)
{
    if (hay)
    {
        for (isize i = StrLen(hay) - 1; i >= 0; --i)
        {
            if (hay[i] == needle)
            {
                return hay + i;
            }
        }
    }
    return 0;
}

inline cstr StrIRChr(cstr hay, char needle)
{
    if (hay)
    {
        needle = ToLower(needle);
        for (isize i = StrLen(hay) - 1; i >= 0; --i)
        {
            if (ToLower(hay[i]) == needle)
            {
                return hay + i;
            }
        }
    }
    return 0;
}

inline cstr StrStr(cstr hay, cstr needle)
{
    if (hay && needle)
    {
        for (isize i = 0; hay[i];)
        {
            for (isize j = 0; needle[j]; ++j)
            {
                if (hay[i + j] != needle[j])
                {
                    goto notfound;
                }
            }
            return hay + i;
        notfound:
            ++i;
        }
    }
    return 0;
}

inline cstr StrRStr(cstr hay, cstr needle)
{
    if (hay && needle)
    {
        const isize jStart = StrLen(needle) - 1;
        for (isize i = StrLen(hay) - 1; i >= 0;)
        {
            for (isize j = 0; needle[j]; ++j)
            {
                if (hay[i + j] != needle[j])
                {
                    goto notfound;
                }
            }
            return hay + i;
        notfound:
            --i;
        }
    }
    return 0;
}

inline cstr StrIStr(cstr hay, cstr needle)
{
    if (hay && needle)
    {
        for (isize i = 0; hay[i];)
        {
            for (isize j = 0; needle[j]; ++j)
            {
                if (ToLower(hay[i + j]) != ToLower(needle[j]))
                {
                    goto notfound;
                }
            }
            return hay + i;
        notfound:
            ++i;
        }
    }
    return 0;
}

inline cstr StrIRStr(cstr hay, cstr needle)
{
    if (hay && needle)
    {
        const isize jStart = StrLen(needle) - 1;
        for (isize i = StrLen(hay) - 1; i >= 0;)
        {
            for (isize j = 0; needle[j]; ++j)
            {
                if (ToLower(hay[i + j]) != ToLower(needle[j]))
                {
                    goto notfound;
                }
            }
            return hay + i;
        notfound:
            --i;
        }
    }
    return 0;
}

inline char* StrCpy(char* dst, usize dstSize, cstr src)
{
    usize i = 0;
    if (dst && src)
    {
        for (; i < dstSize && src[i]; ++i)
        {
            dst[i] = src[i];
        }
        usize nt = i < dstSize ? i : dstSize - 1;
        dst[nt] = 0;
        return dst + i;
    }
    return 0;
}

template<usize dstSize>
inline char* StrCpy(char(&dst)[dstSize], cstr src)
{
    return StrCpy(dst, dstSize, src);
}

inline char* StrRCpy(char* dst, usize dstSize, cstr src)
{
    usize i = 0;
    if (dst && src)
    {
        isize j = StrLen(src) - 1;
        for (; i < dstSize && j >= 0; ++i, --j)
        {
            dst[i] = src[j];
        }
        usize nt = i < dstSize ? i : dstSize - 1;
        dst[nt] = 0;
        return dst + i;
    }
    return 0;
}

template<usize dstSize>
inline char* StrRCpy(char(&dst)[dstSize], cstr src)
{
    return StrRCpy(dst, dstSize, src);
}

inline i32 StrCmp(cstr lhs, cstr rhs)
{
    if (!lhs)
    {
        return rhs ? 1 : 0;
    }
    if (!rhs)
    {
        return -1;
    }
    for (usize i = 0; lhs[i]; ++i)
    {
        char l = lhs[i];
        char r = rhs[i];
        if (l != r)
        {
            return l < r ? -1 : 1;
        }
    }
    return 0;
}

inline i32 StrICmp(cstr lhs, cstr rhs)
{
    if (!lhs)
    {
        return rhs ? 1 : 0;
    }
    if (!rhs)
    {
        return -1;
    }
    for (usize i = 0; lhs[i]; ++i)
    {
        char l = ToLower(lhs[i]);
        char r = ToLower(rhs[i]);
        if (l != r)
        {
            return l < r ? -1 : 1;
        }
    }
    return 0;
}

inline char* StrCat(char* dst, usize dstSize, cstr src)
{
    if (!dst || !src || !dstSize)
    {
        return 0;
    }

    usize start = StrLen(dst);
    usize i = start;
    usize j = 0;
    for (; i < dstSize && src[j]; ++i, ++j)
    {
        dst[i] = src[j];
    }

    usize nt = i < dstSize ? i : dstSize - 1;
    dst[nt] = 0;

    return dst + i;
}

template<usize dstSize>
inline char* StrCat(char(&dst)[dstSize], cstr src)
{
    return StrCat(dst, dstSize, src);
}

inline char* StrRCat(char* dst, usize dstSize, cstr src)
{
    if (!dst || !src || !dstSize)
    {
        return 0;
    }

    usize i = StrLen(dst);
    isize j = StrLen(src) - 1;
    for (; i < dstSize && j >= 0; ++i, --j)
    {
        dst[i] = src[j];
    }

    usize nt = i < dstSize ? i : dstSize - 1;
    dst[nt] = 0;

    return dst + i;
}

template<usize dstSize>
inline char* StrRCat(char(&dst)[dstSize], cstr src)
{
    return StrRCat(dst, dstSize, src);
}

inline char* ReplaceChars(char* dst, char have, char want)
{
    isize last = -1;
    if (dst)
    {
        for (usize i = 0; dst[i]; ++i)
        {
            if (dst[i] == have)
            {
                dst[i] = want;
                last = i;
            }
        }
    }
    return (last != -1) ? dst + last : 0;
}

inline char* RemoveChar(char * dst, usize dstSize, cstr ptr)
{
    cstr end = dst + dstSize;
    if (dst && (ptr >= dst) && (ptr < end))
    {
        for (char* p = (char*)ptr; p + 1 < end; ++p)
        {
            p[0] = p[1];
        }
        return (char*)ptr;
    }
    return 0;
}

template<usize dstSize>
inline char* RemoveChar(char(&dst)[dstSize], cstr ptr)
{
    return RemoveChar(dst, dstSize, ptr);
}

inline char* RemoveStr(char* dst, usize dstSize, cstr ptr)
{
    cstr end = dst + dstSize;
    if (dst && (ptr >= dst) && (ptr < end))
    {
        usize len = StrLen(ptr);
        char* p = (char*)ptr;
        while (p + len < end)
        {
            p[0] = p[len];
            ++p;
        }
        while (p < end)
        {
            *p++ = 0;
        }
        return (char*)ptr;
    }
    return 0;
}

template<usize dstSize>
inline char* RemoveStr(char(&dst)[dstSize], cstr ptr)
{
    return RemoveStr(dst, dstSize, ptr);
}

inline char* RemoveBefore(char* dst, usize dstSize, cstr ptr)
{
    cstr end = dst + dstSize;
    if (dst && (ptr >= dst) && (ptr < end))
    {
        for (char* p = dst; p < ptr; ++p)
        {
            *p = 0;
        }
        StrCpy(dst, dstSize, ptr);
        return (char*)ptr;
    }
    return 0;
}

template<usize dstSize>
inline char* RemoveBefore(char(&dst)[dstSize], cstr ptr)
{
    return RemoveBefore(dst, dstSize, ptr);
}

inline char* RemoveAfter(char* dst, usize dstSize, cstr ptr)
{
    cstr end = dst + dstSize;
    if (dst && (ptr >= dst) && (ptr < end))
    {
        for (char* p = (char*)ptr + 1; p < end; ++p)
        {
            *p = 0;
        }
        return (char*)ptr;
    }
    return 0;
}

template<usize dstSize>
inline char* RemoveAfter(char(&dst)[dstSize], cstr ptr)
{
    return RemoveAfter(dst, dstSize, ptr);
}

inline cstr BeginsWithChar(cstr x, char ch)
{
    if (x)
    {
        if (x[0] == ch)
        {
            return x;
        }
    }
    return 0;
}

inline cstr BeginsWithStr(cstr x, cstr y)
{
    if (x && y)
    {
        for (isize i = 0; y[i]; ++i)
        {
            if (y[i] != x[i])
            {
                return 0;
            }
        }
        return x;
    }
    return 0;
}

inline cstr EndsWithChar(cstr x, char ch)
{
    cstr back = StrBack(x);
    if (back)
    {
        return *back == ch ? back : 0;
    }
    return 0;
}

inline cstr EndsWithStr(cstr x, cstr y)
{
    if (x && y)
    {
        isize xlen = StrLen(x);
        isize ylen = StrLen(y);
        isize idx = (xlen - ylen);
        if (idx >= 0)
        {
            if (StrCmp(x + idx, y) == 0)
            {
                return x + idx;
            }
        }
    }
    return 0;
}

