#include "common/stringutil.h"

#include "common/macro.h"
#include "common/minmax.h"
#include "common/valist.h"
#include <stdio.h>

// ----------------------------------------------------------------------------
// null termination

i32 NullTerminate(char* dst, i32 size, i32 length)
{
    i32 nt = 0;

    ASSERT(dst || !size);
    ASSERT(length >= 0);
    ASSERT(size >= 0);

    if (size > 0)
    {
        nt = Min(length, size - 1);
        dst[nt] = '\0';
    }

    return nt;
}

// ----------------------------------------------------------------------------
// case conversion

char ToUpper(char x)
{
    i32 c = (i32)x;
    i32 lo = ('a' - 1) - c;     // c >= 'a'
    i32 hi = c - ('z' + 1);     // c <= 'z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

char ToLower(char x)
{
    i32 c = (i32)x;
    i32 lo = ('A' - 1) - c;     // c >= 'A'
    i32 hi = c - ('Z' + 1);     // c <= 'Z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('a' - 'A'));
    return (char)c;
}

i32 ToLower(char* dst, i32 size)
{
    i32 i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ToLower(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

i32 ToUpper(char* dst, i32 size)
{
    i32 i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ToUpper(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

// ----------------------------------------------------------------------------
// string length

i32 StrLen(cstrc x)
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

i32 StrLen(cstrc x, i32 size)
{
    i32 i = 0;
    ASSERT(size >= 0);
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

i32 StrCpy(char* dst, i32 size, cstr src)
{
    i32 i = 0;

    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    for (; i < size && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, i);
}

i32 StrCat(char* dst, i32 size, cstr src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    const i32 len = StrLen(dst, size);
    return StrCpy(dst + len, size - len, src) + len;
}

i32 ShiftRight(char* dst, i32 size, i32 shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const i32 len = Min(size, StrLen(dst, size) + shifts);
    cstr src = dst - shifts;
    for (i32 i = len - 1; i >= shifts; --i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, len);
}

i32 ShiftLeft(char* dst, i32 size, i32 shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const i32 len = Max(0, StrLen(dst, size) - shifts);
    cstr src = dst + shifts;
    for (i32 i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, len);
}

i32 Shift(char* dst, i32 size, i32 shifts)
{
    return (shifts < 0) ?
        ShiftLeft(dst, size, -shifts) :
        ShiftRight(dst, size, shifts);
}

// ----------------------------------------------------------------------------
// string compare

i32 StrCmp(cstrc lhs, i32 size, cstrc rhs)
{
    i32 c = 0;

    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        char L = lhs[i];
        char R = rhs[i];
        c = L - R;
        if (c || !L)
        {
            break;
        }
    }

    return c;
}

i32 StrICmp(cstrc lhs, i32 size, cstrc rhs)
{
    i32 c = 0;

    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        char L = lhs[i];
        char R = rhs[i];
        c = ToLower(L) - ToLower(R);
        if (c || !L)
        {
            break;
        }
    }

    return c;
}

// ----------------------------------------------------------------------------
// string searching

cstr StrChr(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        if (!hay[i])
        {
            break;
        }
        if (hay[i] == needle)
        {
            ptr = hay + i;
            break;
        }
    }

    return nullptr;
}

cstr StrIChr(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    needle = ToLower(needle);
    for (i32 i = 0; i < size; ++i)
    {
        if (!hay[i])
        {
            break;
        }
        if (ToLower(hay[i]) == needle)
        {
            ptr = hay + i;
            break;
        }
    }

    return nullptr;
}

cstr StrStr(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        i32 c = 0;
        for (i32 j = 0; !c && needle[j]; ++j)
        {
            c = hay[i + j] - needle[j];
        }
        if (!c)
        {
            ptr = hay + i;
            break;
        }
    }

    return ptr;
}

cstr StrIStr(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        i32 c = 0;
        for (i32 j = 0; !c && needle[j]; ++j)
        {
            c = ToLower(hay[i + j]) - ToLower(needle[j]);
        }
        if (!c)
        {
            ptr = hay + i;
            break;
        }
    }

    return ptr;
}

cstr StartsWith(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        char H = hay[i];
        char N = needle[i];
        if (!N)
        {
            ptr = hay;
            break;
        }
        if (H != N)
        {
            break;
        }
    }
    return ptr;
}

cstr IStartsWith(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        char H = ToLower(hay[i]);
        char N = ToLower(needle[i]);
        if (!N)
        {
            ptr = hay;
            break;
        }
        if (H != N)
        {
            break;
        }
    }
    return ptr;
}

cstr StartsWith(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    if (size > 0)
    {
        if (hay[0] == needle)
        {
            ptr = hay;
        }
    }
    return ptr;
}

cstr IStartsWith(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    if (size > 0)
    {
        if (ToLower(hay[0]) == ToLower(needle))
        {
            ptr = hay;
        }
    }
    return ptr;
}

cstr EndsWith(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 i = hayLen - needleLen;

    if (i >= 0)
    {
        if (!StrCmp(hay + i, needleLen, needle))
        {
            ptr = hay + i;
        }
    }

    return ptr;
}

cstr IEndsWith(cstr hay, i32 size, cstr needle)
{
    cstr ptr = nullptr;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 i = hayLen - needleLen;

    if (i >= 0)
    {
        if (!StrICmp(hay + i, needleLen, needle))
        {
            ptr = hay + i;
        }
    }

    return ptr;
}

cstr EndsWith(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    const i32 back = StrLen(hay, size) - 1;
    if (back >= 0)
    {
        if (hay[back] == needle)
        {
            ptr = hay + back;
        }
    }
    return ptr;
}

cstr IEndsWith(cstr hay, i32 size, char needle)
{
    cstr ptr = nullptr;

    const i32 back = StrLen(hay, size) - 1;
    if (back >= 0)
    {
        if (ToLower(hay[back]) == ToLower(needle))
        {
            ptr = hay + back;
        }
    }
    return ptr;
}

// ----------------------------------------------------------------------------
// string formatting

i32 VSPrintf(char* dst, i32 size, cstr fmt, VaList va)
{
    i32 len = 0;

    ASSERT((dst && fmt) || !size);
    ASSERT(size >= 0);
    ASSERT(va);

    if (size > 0)
    {
        fmt = fmt ? fmt : "(NULL)";
        const i32 result = vsnprintf(dst, size, fmt, va);
        const i32 wrote = result < 0 ? size : result;
        len = NullTerminate(dst, size, wrote);
    }

    return len;
}

i32 SPrintf(char* dst, i32 size, cstr fmt, ...)
{
    return VSPrintf(dst, size, fmt, VaStart(fmt));
}

i32 StrCatf(char* dst, i32 size, cstr fmt, ...)
{
    i32 len = StrLen(dst, size);
    return VSPrintf(dst + len, size - len, fmt, VaStart(fmt)) + len;
}

// ----------------------------------------------------------------------------
// char replace

i32 ChrRep(char* dst, i32 size, char fnd, char rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (dst[i] == fnd) ? rep : dst[i];
    }
    return NullTerminate(dst, size, i);
}

i32 ChrIRep(char* dst, i32 size, char fnd, char rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    fnd = ToLower(fnd);
    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (ToLower(dst[i]) == fnd) ? rep : dst[i];
    }
    return NullTerminate(dst, size, i);
}

// ----------------------------------------------------------------------------
// string replace

i32 StrRep(char* dst, i32 size, cstr fnd, cstr rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const i32 dstLen = StrLen(dst, size);
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);
    const i32 lenDiff = repLen - fndLen;

    i32 count = 0;

    char* inst = (char*)StrStr(dst, size, fnd);
    while (inst)
    {
        ++count;

        const i32 i = (i32)(inst - dst);
        const i32 remLen = size - i;
        Shift(inst, remLen, lenDiff);

        const i32 cpyLen = Min(remLen, repLen);
        for (i32 j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }

        inst = (char*)StrStr(inst + 1, remLen - 1, fnd);
    }

    const i32 newLen = dstLen + count * lenDiff;
    return NullTerminate(dst, size, newLen);
}

i32 StrIRep(char* dst, i32 size, cstr fnd, cstr rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const i32 dstLen = StrLen(dst, size);
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);
    const i32 lenDiff = repLen - fndLen;

    i32 count = 0;

    char* inst = (char*)StrIStr(dst, size, fnd);
    while (inst)
    {
        ++count;

        const i32 i = (i32)(inst - dst);
        const i32 remLen = size - i;
        Shift(inst, remLen, lenDiff);

        const i32 cpyLen = Min(remLen, repLen);
        for (i32 j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }

        inst = (char*)StrIStr(inst + 1, remLen - 1, fnd);
    }

    const i32 newLen = dstLen + count * lenDiff;
    return NullTerminate(dst, size, newLen);
}

// ----------------------------------------------------------------------------
// paths

char FixPath(char c)
{
    switch (c)
    {
    IF_WIN32(case '/': return '\\';)
    IF_UNIX(case '\\': return '/';)
    default: return c;
    };
}

i32 FixPath(char* dst, i32 size)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = FixPath(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

i32 PathCpy(char* dst, i32 size, cstr src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    StrCpy(dst, size, src);
    return FixPath(dst, size);
}

i32 PathCat(char* dst, i32 size, cstr src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    i32 len = StrLen(dst, size);
    return PathCpy(dst + len, size - len, src) + len;
}
