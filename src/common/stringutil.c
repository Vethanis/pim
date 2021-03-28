#include "common/stringutil.h"

#include "common/macro.h"
#include "allocator/allocator.h"
#include "stb/stb_sprintf.h"
#include <string.h>
#include <stdarg.h>

pim_inline i32 min_i32(i32 a, i32 b)
{
    return (a < b) ? a : b;
}

pim_inline i32 max_i32(i32 a, i32 b)
{
    return (a > b) ? a : b;
}

// ----------------------------------------------------------------------------
// null termination

i32 NullTerminate(char *const dst, i32 size, i32 length)
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

i32 StrUp(char *const dst, i32 size)
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

i32 StrLo(char *const dst, i32 size)
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

i32 StrLen(char const *const x)
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

i32 StrNLen(char const *const x, i32 size)
{
    i32 i = 0;
    ASSERT(size >= 0);
    if (x)
    {
        while ((i < size) && x[i])
        {
            ++i;
        }
    }
    return i;
}

// ----------------------------------------------------------------------------
// string copying

i32 StrCpy(char *const dst, i32 size, char const *const src)
{
    i32 i = 0;

    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    for (; (i < size) && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, i);
}

i32 StrCat(char *const dst, i32 size, char const *const src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    const i32 len = StrNLen(dst, size);
    return StrCpy(dst + len, size - len, src) + len;
}

char* StrDup(char const *const src, EAlloc allocator)
{
    if (src && src[0])
    {
        const i32 len = StrLen(src);
        char* dst = Mem_Alloc(allocator, len + 1);
        memcpy(dst, src, len);
        dst[len] = 0;
        return dst;
    }
    return NULL;
}

i32 ShiftRight(char *const dst, i32 size, i32 shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const i32 len = min_i32(size, StrNLen(dst, size) + shifts);
    char const *const src = dst - shifts;
    for (i32 i = len - 1; i >= shifts; --i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, len);
}

i32 ShiftLeft(char *const dst, i32 size, i32 shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const i32 len = max_i32(0, StrNLen(dst, size) - shifts);
    char const *const src = dst + shifts;
    memmove(dst, src, len);
    return NullTerminate(dst, size, len);
}

i32 Shift(char *const dst, i32 size, i32 shifts)
{
    if (!shifts)
    {
        return StrNLen(dst, size);
    }
    return (shifts < 0) ?
        ShiftLeft(dst, size, -shifts) :
        ShiftRight(dst, size, shifts);
}

// ----------------------------------------------------------------------------
// string compare

i32 StrCmp(char const *const lhs, i32 size, char const *const rhs)
{
    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    i32 c = 0;
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

i32 StrICmp(char const *const lhs, i32 size, char const *const rhs)
{
    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    i32 c = 0;
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

i32 MemICmp(char const *const pim_noalias lhs, char const *const pim_noalias rhs, i32 size)
{
    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    i32 c = 0;
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

const char* StrChr(char const *const hay, i32 size, char needle)
{
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
            return hay + i;
            break;
        }
    }

    return NULL;
}

const char* StrIChr(char const *const hay, i32 size, char needle)
{
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
            return hay + i;
            break;
        }
    }

    return NULL;
}

const char* StrRChr(char const *const hay, i32 size, char needle)
{
    ASSERT(hay || !size);
    ASSERT(size >= 0);

    const i32 len = StrNLen(hay, size);
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (hay[i] == needle)
        {
            return hay + i;
            break;
        }
    }

    return NULL;
}

const char* StrRIChr(char const *const hay, i32 size, char needle)
{
    ASSERT(hay || !size);
    ASSERT(size >= 0);

    const i32 len = StrNLen(hay, size);
    needle = ChrLo(needle);
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (ChrLo(hay[i]) == needle)
        {
            return hay + i;
            break;
        }
    }

    return NULL;
}

const char* StrStr(char const *const hay, i32 size, char const *const needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    i32 hayIndex = 0;
    while (hlen >= nlen)
    {
        if (!memcmp(hay + hayIndex, needle, nlen))
        {
            return hay + hayIndex;
        }
        ++hayIndex;
        --hlen;
    }

    return NULL;
}

const char* StrIStr(char const *const hay, i32 size, char const *const needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    i32 hlen = StrNLen(hay, size);
    i32 nlen = StrLen(needle);
    i32 hayIndex = 0;
    while (hlen >= nlen)
    {
        if (!MemICmp(hay + hayIndex, needle, nlen))
        {
            return hay + hayIndex;
        }
        ++hayIndex;
        --hlen;
    }

    return NULL;
}

const char* StartsWith(char const *const hay, i32 size, char const *const needle)
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

const char* IStartsWith(char const *const hay, i32 size, char const *const needle)
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

const char* EndsWith(char const *const hay, i32 size, char const *const needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrNLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 index = hayLen - needleLen;
    if (index >= 0)
    {
        if (!StrCmp(hay + index, needleLen, needle))
        {
            return hay + index;
        }
    }

    return NULL;
}

const char* IEndsWith(char const *const hay, i32 size, char const *const needle)
{
    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const i32 hayLen = StrNLen(hay, size);
    const i32 needleLen = StrLen(needle);
    const i32 index = hayLen - needleLen;
    if (index >= 0)
    {
        if (!StrICmp(hay + index, needleLen, needle))
        {
            return hay + index;
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------------
// string formatting

i32 VSPrintf(char *const dst, i32 size, char const *const fmt, va_list va)
{
    ASSERT((dst && fmt) || !size);
    ASSERT(size >= 0);
    ASSERT(va);

    i32 len = 0;
    if (size > 0)
    {
        len = stbsp_vsnprintf(dst, size, fmt ? fmt : "(NULL)", va);
        len = NullTerminate(dst, size, len);
    }

    return len;
}

i32 SPrintf(char *const dst, i32 size, char const *const fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    i32 rv = VSPrintf(dst, size, fmt, ap);
    va_end(ap);
    return rv;
}

i32 VStrCatf(char *const dst, i32 size, char const *const fmt, va_list va)
{
    i32 len = StrNLen(dst, size);
    return VSPrintf(dst + len, size - len, fmt, va) + len;
}

i32 StrCatf(char *const dst, i32 size, char const *const fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    i32 len = StrNLen(dst, size);
    i32 rv = VSPrintf(dst + len, size - len, fmt, ap) + len;
    va_end(ap);
    return rv;
}

// ----------------------------------------------------------------------------
// char replace

i32 ChrRep(char *const dst, i32 size, char fnd, char rep)
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

i32 ChrIRep(char *const dst, i32 size, char fnd, char rep)
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

i32 StrRep(char *const dst, i32 size, char const *const fnd, char const *const rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(fnd);
    ASSERT(rep);

    const i32 dstLen = StrNLen(dst, size);
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);

    i32 curLen = dstLen;
    char* inst = (char*)StrStr(dst, size, fnd);
    while (inst)
    {
        i32 index = (i32)(inst - dst);
        i32 remSize = size - index;
        Shift(inst, remSize, repLen - fndLen);

        i32 cpyLen = min_i32(remSize, repLen);
        for (i32 i = 0; i < cpyLen; ++i)
        {
            inst[i] = rep[i];
        }
        curLen += cpyLen - fndLen;

        inst += cpyLen;
        index = (i32)(inst - dst);
        i32 remLen = curLen - index;
        inst = (char*)StrStr(inst, remLen, fnd);
    }

    return NullTerminate(dst, size, curLen);
}

i32 StrIRep(char *const dst, i32 size, char const *const fnd, char const *const rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(fnd);
    ASSERT(rep);

    const i32 dstLen = StrNLen(dst, size);
    const i32 fndLen = StrLen(fnd);
    const i32 repLen = StrLen(rep);

    i32 curLen = dstLen;
    char* inst = (char*)StrIStr(dst, size, fnd);
    while (inst)
    {
        i32 index = (i32)(inst - dst);
        i32 remSize = size - index;
        Shift(inst, remSize, repLen - fndLen);

        i32 cpyLen = min_i32(remSize, repLen);
        for (i32 i = 0; i < cpyLen; ++i)
        {
            inst[i] = rep[i];
        }
        curLen += cpyLen - fndLen;

        inst += cpyLen;
        index = (i32)(inst - dst);
        i32 remLen = curLen - index;
        inst = (char*)StrIStr(inst, remLen, fnd);
    }

    return NullTerminate(dst, size, curLen);
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

i32 StrPath(char *const dst, i32 size)
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
