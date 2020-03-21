#include "common/stringutil.h"

#include "common/macro.h"
#include "common/valist.h"
#include "stb/stb_sprintf.h"

static int32_t min_i32(int32_t a, int32_t b)
{
    return (a < b) ? a : b;
}

static int32_t max_i32(int32_t a, int32_t b)
{
    return (a > b) ? a : b;
}

// ----------------------------------------------------------------------------
// null termination

int32_t NullTerminate(char* dst, int32_t size, int32_t length)
{
    int32_t nt = 0;

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
    int32_t c = (int32_t)x;
    int32_t lo = ('a' - 1) - c;     // c >= 'a'
    int32_t hi = c - ('z' + 1);     // c <= 'z'
    int32_t mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('A' - 'a'));
    return (char)c;
}

char ChrLo(char x)
{
    int32_t c = (int32_t)x;
    int32_t lo = ('A' - 1) - c;     // c >= 'A'
    int32_t hi = c - ('Z' + 1);     // c <= 'Z'
    int32_t mask = (lo & hi) >> 9;  // &&
    c = c + (mask & ('a' - 'A'));
    return (char)c;
}

int32_t StrUp(char* dst, int32_t size)
{
    int32_t i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ChrUp(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

int32_t StrLo(char* dst, int32_t size)
{
    int32_t i = 0;

    ASSERT(dst || !size);
    ASSERT(size >= 0);

    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ChrLo(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

// ----------------------------------------------------------------------------
// string length

int32_t StrLen(const char* x)
{
    int32_t i = 0;
    if (x)
    {
        while (x[i])
        {
            ++i;
        }
    }
    return i;
}

int32_t StrNLen(const char* x, int32_t size)
{
    int32_t i = 0;
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

int32_t StrCpy(char* dst, int32_t size, const char* src)
{
    int32_t i = 0;

    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    for (; i < size && src[i]; ++i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, i);
}

int32_t StrCat(char* dst, int32_t size, const char* src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    const int32_t len = StrNLen(dst, size);
    return StrCpy(dst + len, size - len, src) + len;
}

int32_t ShiftRight(char* dst, int32_t size, int32_t shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const int32_t len = min_i32(size, StrNLen(dst, size) + shifts);
    const char* src = dst - shifts;
    for (int32_t i = len - 1; i >= shifts; --i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, len);
}

int32_t ShiftLeft(char* dst, int32_t size, int32_t shifts)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);
    ASSERT(shifts >= 0);

    const int32_t len = max_i32(0, StrNLen(dst, size) - shifts);
    const char* src = dst + shifts;
    for (int32_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
    return NullTerminate(dst, size, len);
}

int32_t Shift(char* dst, int32_t size, int32_t shifts)
{
    return (shifts < 0) ?
        ShiftLeft(dst, size, -shifts) :
        ShiftRight(dst, size, shifts);
}

// ----------------------------------------------------------------------------
// string compare

int32_t StrCmp(const char* lhs, int32_t size, const char* rhs)
{
    int32_t c = 0;

    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
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

int32_t StrICmp(const char* lhs, int32_t size, const char* rhs)
{
    int32_t c = 0;

    ASSERT((lhs && rhs) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
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

// ----------------------------------------------------------------------------
// string searching

const char* StrChr(const char* hay, int32_t size, char needle)
{
    const char* ptr = 0;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
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

const char* StrIChr(const char* hay, int32_t size, char needle)
{
    const char* ptr = 0;

    ASSERT(hay || !size);
    ASSERT(size >= 0);

    needle = ChrLo(needle);
    for (int32_t i = 0; i < size; ++i)
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

const char* StrStr(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
    {
        int32_t c = 0;
        for (int32_t j = 0; !c && needle[j]; ++j)
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

const char* StrIStr(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
    {
        int32_t c = 0;
        for (int32_t j = 0; !c && needle[j]; ++j)
        {
            c = ChrLo(hay[i + j]) - ChrLo(needle[j]);
        }
        if (!c)
        {
            ptr = hay + i;
            break;
        }
    }

    return ptr;
}

const char* StartsWith(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
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

const char* IStartsWith(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    for (int32_t i = 0; i < size; ++i)
    {
        char H = ChrLo(hay[i]);
        char N = ChrLo(needle[i]);
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

const char* EndsWith(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const int32_t hayLen = StrNLen(hay, size);
    const int32_t needleLen = StrLen(needle);
    const int32_t i = hayLen - needleLen;

    if (i >= 0)
    {
        if (!StrCmp(hay + i, needleLen, needle))
        {
            ptr = hay + i;
        }
    }

    return ptr;
}

const char* IEndsWith(const char* hay, int32_t size, const char* needle)
{
    const char* ptr = 0;

    ASSERT((hay && needle) || !size);
    ASSERT(size >= 0);

    const int32_t hayLen = StrNLen(hay, size);
    const int32_t needleLen = StrLen(needle);
    const int32_t i = hayLen - needleLen;

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

int32_t VSPrintf(char* dst, int32_t size, const char* fmt, VaList va)
{
    int32_t len = 0;

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

int32_t SPrintf(char* dst, int32_t size, const char* fmt, ...)
{
    return VSPrintf(dst, size, fmt, VA_START(fmt));
}

int32_t StrCatf(char* dst, int32_t size, const char* fmt, ...)
{
    int32_t len = StrNLen(dst, size);
    return VSPrintf(dst + len, size - len, fmt, VA_START(fmt)) + len;
}

// ----------------------------------------------------------------------------
// char replace

int32_t ChrRep(char* dst, int32_t size, char fnd, char rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    int32_t i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (dst[i] == fnd) ? rep : dst[i];
    }
    return NullTerminate(dst, size, i);
}

int32_t ChrIRep(char* dst, int32_t size, char fnd, char rep)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    fnd = ChrLo(fnd);
    int32_t i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = (ChrLo(dst[i]) == fnd) ? rep : dst[i];
    }
    return NullTerminate(dst, size, i);
}

// ----------------------------------------------------------------------------
// string replace

int32_t StrRep(char* dst, int32_t size, const char* fnd, const char* rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const int32_t dstLen = StrNLen(dst, size);
    const int32_t fndLen = StrLen(fnd);
    const int32_t repLen = StrLen(rep);
    const int32_t lenDiff = repLen - fndLen;

    int32_t count = 0;

    char* inst = (char*)StrStr(dst, size, fnd);
    while (inst)
    {
        ++count;

        const int32_t i = (int32_t)(inst - dst);
        const int32_t remLen = size - i;
        Shift(inst, remLen, lenDiff);

        const int32_t cpyLen = min_i32(remLen, repLen);
        for (int32_t j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }

        inst = (char*)StrStr(inst + 1, remLen - 1, fnd);
    }

    const int32_t newLen = dstLen + count * lenDiff;
    return NullTerminate(dst, size, newLen);
}

int32_t StrIRep(char* dst, int32_t size, const char* fnd, const char* rep)
{
    ASSERT((dst && fnd && rep) || !size);
    ASSERT(size >= 0);

    const int32_t dstLen = StrNLen(dst, size);
    const int32_t fndLen = StrLen(fnd);
    const int32_t repLen = StrLen(rep);
    const int32_t lenDiff = repLen - fndLen;

    int32_t count = 0;

    char* inst = (char*)StrIStr(dst, size, fnd);
    while (inst)
    {
        ++count;

        const int32_t i = (int32_t)(inst - dst);
        const int32_t remLen = size - i;
        Shift(inst, remLen, lenDiff);

        const int32_t cpyLen = min_i32(remLen, repLen);
        for (int32_t j = 0; j < cpyLen; ++j)
        {
            inst[j] = rep[j];
        }

        inst = (char*)StrIStr(inst + 1, remLen - 1, fnd);
    }

    const int32_t newLen = dstLen + count * lenDiff;
    return NullTerminate(dst, size, newLen);
}

// ----------------------------------------------------------------------------
// paths

char ChrPath(char c)
{
    switch (c)
    {
    IF_WIN(case '/': return '\\';)
    IF_UNIX(case '\\': return '/';)
    default: return c;
    };
}

int32_t StrPath(char* dst, int32_t size)
{
    ASSERT(dst || !size);
    ASSERT(size >= 0);

    int32_t i = 0;
    for (; i < size && dst[i]; ++i)
    {
        dst[i] = ChrPath(dst[i]);
    }
    return NullTerminate(dst, size, i);
}

int32_t PathCpy(char* dst, int32_t size, const char* src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    StrCpy(dst, size, src);
    return StrPath(dst, size);
}

int32_t PathCat(char* dst, int32_t size, const char* src)
{
    ASSERT((dst && src) || !size);
    ASSERT(size >= 0);

    int32_t len = StrNLen(dst, size);
    return PathCpy(dst + len, size - len, src) + len;
}
