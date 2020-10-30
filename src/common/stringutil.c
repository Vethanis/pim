#include "common/stringutil.h"

#include "common/macro.h"
#include "common/valist.h"
#include "allocator/allocator.h"
#include "stb/stb_sprintf.h"
#include <string.h>

static i32 min_i32(i32 a, i32 b)
{
    return (a < b) ? a : b;
}

static i32 max_i32(i32 a, i32 b)
{
    return (a > b) ? a : b;
}

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
        nt = min_i32(length, size - 1);
        dst[nt] = '\0';
    }

    return nt;
}

// ----------------------------------------------------------------------------
// case conversion

char ChrUp(char x)
{
    i32 c = (i32)x;
    i32 lo = ('a' - 1) - c;     // c >= 'a'
    i32 hi = c - ('z' + 1);     // c <= 'z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

char ChrLo(char x)
{
    i32 c = (i32)x;
    i32 lo = ('A' - 1) - c;     // c >= 'A'
    i32 hi = c - ('Z' + 1);     // c <= 'Z'
    i32 mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('a' - 'A'));
    return (char)c;
}

i32 StrUp(char* dst, i32 size)
{
    i32 i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ChrUp(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

i32 StrLo(char* dst, i32 size)
{
    i32 i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ChrLo(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

bool IsAlpha(char c)
{
    c = ChrLo(c);
    return (c >= 'a') && (c <= 'z');
}

bool IsDigit(char c)
{
    return (c >= '0') && (c <= '9');
}

bool IsSpace(char c)
{
    return (c > 0) && (c <= 32);
}

// ----------------------------------------------------------------------------
// string length

i32 StrLen(const char* x)
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

i32 StrNLen(const char* x, i32 size)
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

i32 StrCpy(char* dst, i32 size, const char* src)
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

i32 StrCat(char* dst, i32 size, const char* src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    const i32 len = StrNLen(dst, size);
    return StrCpy(dst + len, size - len, src) + len;
}

char* StrDup(const char* src, EAlloc allocator)
{
    if (src && src[0])
    {
        const i32 len = StrLen(src);
        char* dst = pim_malloc(allocator, len + 1);
        memcpy(dst, src, len);
        dst[len] = 0;
        return dst;
    }
    return NULL;
}

i32 ShiftRight(char* dst, i32 size, i32 shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const i32 len = min_i32(size, StrNLen(dst, size) + shifts);
    const char* src = dst - shifts;
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

    const i32 len = max_i32(0, StrNLen(dst, size) - shifts);
    const char* src = dst + shifts;
    memmove(dst, src, len);
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

i32 StrCmp(const char* lhs, i32 size, const char* rhs)
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

i32 StrICmp(const char* lhs, i32 size, const char* rhs)
{
    i32 c = 0;

    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    for (i32 i = 0; i < size; ++i)
    {
        char L = lhs[i];
        char R = rhs[i];
        c = ChrLo(L) - ChrLo(R);
        if (c || !L)
        {
            break;
        }
    }

    return c;
}

i32 MemICmp(const char* pim_noalias lhs, const char* pim_noalias rhs, i32 size)
{
    i32 c = 0;
    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);
    for (i32 i = 0; i < size; ++i)
    {
        char L = lhs[i];
        char R = rhs[i];
        c = ChrLo(L) - ChrLo(R);
        if (c)
        {
            break;
        }
    }
    return c;
}

// ----------------------------------------------------------------------------
// string searching

const char* StrChr(const char* hay, i32 size, char needle)
{
    const char* ptr = 0;

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

    return 0;
}

const char* StrIChr(const char* hay, i32 size, char needle)
{
    const char* ptr = 0;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    needle = ChrLo(needle);
    for (i32 i = 0; i < size; ++i)
    {
        if (!hay[i])
        {
            break;
        }
        if (ChrLo(hay[i]) == needle)
        {
            ptr = hay + i;
            break;
        }
    }

    return 0;
}

const char* StrStr(const char* hay, i32 size, const char* needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    while (hlen >= nlen)
    {
        if (!memcmp(hay, needle, nlen))
        {
            return hay;
        }
        ++hay;
        --hlen;
    }

    return NULL;
}

const char* StrIStr(const char* hay, i32 size, const char* needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    while (hlen >= nlen)
    {
        if (!MemICmp(hay, needle, nlen))
        {
            return hay;
        }
        ++hay;
        --hlen;
    }

    return NULL;
}

const char* StartsWith(const char* hay, i32 size, const char* needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    if (hlen < nlen)
    {
        return NULL;
    }
    if (!StrCmp(hay, nlen, needle))
    {
        return hay;
    }
    return NULL;
}

const char* IStartsWith(const char* hay, i32 size, const char* needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    if (hlen < nlen)
    {
        return NULL;
    }
    if (!StrICmp(hay, nlen, needle))
    {
        return hay;
    }
    return NULL;
}

const char* EndsWith(const char* hay, i32 size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrNLen(hay, size);
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

const char* IEndsWith(const char* hay, i32 size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrNLen(hay, size);
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

// ----------------------------------------------------------------------------
// string formatting

i32 VSPrintf(char* dst, i32 size, const char* fmt, VaList va)
{
    i32 len = 0;

    ASSERT((dst && fmt) || !size);
    ASSERT(size >= 0);
    ASSERT(va);

    if (size > 0)
    {
        fmt = fmt ? fmt : "(NULL)";
        len = stbsp_vsnprintf(dst, size, fmt, va);
        len = NullTerminate(dst, size, len);
    }

    return len;
}

i32 SPrintf(char* dst, i32 size, const char* fmt, ...)
{
    return VSPrintf(dst, size, fmt, VA_START(fmt));
}

i32 VStrCatf(char* dst, i32 size, const char* fmt, VaList va)
{
    i32 len = StrNLen(dst, size);
    return VSPrintf(dst + len, size - len, fmt, va) + len;
}

i32 StrCatf(char* dst, i32 size, const char* fmt, ...)
{
    i32 len = StrNLen(dst, size);
    return VSPrintf(dst + len, size - len, fmt, VA_START(fmt)) + len;
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

    fnd = ChrLo(fnd);
    i32 i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (ChrLo(dst[i]) == fnd) ? rep : dst[i];
    }
    return NullTerminate(dst, size, i);
}

// ----------------------------------------------------------------------------
// string replace

i32 StrRep(char* dst, i32 size, const char* fnd, const char* rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const i32 dstLen = StrNLen(dst, size);
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

        const i32 cpyLen = min_i32(remLen, repLen);
        for (i32 j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }

        inst = (char*)StrStr(inst + 1, remLen - 1, fnd);
    }

    const i32 newLen = dstLen + count * lenDiff;
    return NullTerminate(dst, size, newLen);
}

i32 StrIRep(char* dst, i32 size, const char* fnd, const char* rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const i32 dstLen = StrNLen(dst, size);
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

        const i32 cpyLen = min_i32(remLen, repLen);
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

static bool IsPathSeparator(char c)
{
    return (c == '/') || (c == '\\');
}

char ChrPath(char c)
{
    if (IsPathSeparator(c))
    {
        IF_WIN(return '\\');
        IF_UNIX(return '/');
    }
    return c;
}

i32 StrPath(char* dst, i32 size)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    i32 len = StrNLen(dst, size);
    if (len > 0)
    {
        dst[0] = ChrPath(dst[0]);
    }
    for (i32 i = 1; i < len; ++i)
    {
        dst[i] = ChrPath(dst[i]);
        // this nukes network paths; make/use StrNetPath() if network path is desired
        if (IsPathSeparator(dst[i - 1]) && IsPathSeparator(dst[i]))
        {
            for (i32 j = i; j < len; ++j)
            {
                dst[j - 1] = dst[j];
            }
            --i;
            --len;
        }
    }

    return NullTerminate(dst, size, len);
}
