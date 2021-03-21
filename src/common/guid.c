#include "common/guid.h"
#include "common/fnv1a.h"
#include "common/random.h"
#include "common/stringutil.h"
#include "common/time.h"
#include "common/atomics.h"
#include "containers/dict.h"
#include "containers/text.h"

static bool ms_init;
static dict_t ms_guidToStr;
static u32 ms_counter;

static void EnsureInit(void)
{
    if (!ms_init)
    {
        ms_init = true;
        dict_new(&ms_guidToStr, sizeof(Guid), sizeof(text32), EAlloc_Perm);
    }
}

bool guid_isnull(Guid x)
{
    return !(x.a | x.b);
}

bool guid_eq(Guid lhs, Guid rhs)
{
    return !((lhs.a - rhs.a) | (lhs.b - rhs.b));
}

i32 guid_cmp(Guid lhs, Guid rhs)
{
    if (lhs.a != rhs.a)
    {
        return lhs.a < rhs.a ? -1 : 1;
    }
    if (lhs.b != rhs.b)
    {
        return lhs.b < rhs.b ? -1 : 1;
    }
    return 0;
}

Guid guid_new(void)
{
    u64 a = Time_Now();
    u64 b = inc_u32(&ms_counter, MO_Relaxed);
    u64 c = inc_u32(&ms_counter, MO_Relaxed);
    b = b << 32 | c;
    return (Guid) { a, b };
}

void guid_set_name(Guid id, const char* str)
{
    EnsureInit();
    if (str && str[0])
    {
        text32 text = { 0 };
        StrCpy(ARGS(text.c), str);
        if (!dict_add(&ms_guidToStr, &id, &text))
        {
            dict_set(&ms_guidToStr, &id, &text);
        }
    }
    else
    {
        dict_rm(&ms_guidToStr, &id, NULL);
    }
}

bool guid_get_name(Guid id, char* dst, i32 size)
{
    EnsureInit();
    text32 text = { 0 };
    if (dict_get(&ms_guidToStr, &id, &text))
    {
        StrCpy(dst, size, text.c);
        return true;
    }
    dst[0] = 0;
    return false;
}

i32 guid_find(Guid const *const pim_noalias ptr, i32 count, Guid key)
{
    ASSERT(ptr || !count);
    ASSERT(count >= 0);
    for (i32 i = 0; i < count; ++i)
    {
        if (guid_eq(ptr[i], key))
        {
            return i;
        }
    }
    return -1;
}

Guid guid_str(char const *const pim_noalias str)
{
    Guid id = { 0 };
    if (str && str[0])
    {
        u64 a = Fnv64String(str, Fnv64Bias);
        a = a ? a : 1;
        u64 b = Fnv64String(str, a);
        b = b ? b : 1;
        id.a = a;
        id.b = b;
        guid_set_name(id, str);
    }
    return id;
}

Guid guid_bytes(void const *const pim_noalias ptr, i32 nBytes)
{
    Guid id = { 0 };
    if (ptr && nBytes > 0)
    {
        u64 a = Fnv64Bytes(ptr, nBytes, Fnv64Bias);
        a = a ? a : 1;
        u64 b = Fnv64Bytes(ptr, nBytes, a);
        b = b ? b : 1;
        id.a = a;
        id.b = b;
    }
    return id;
}

Guid guid_rand(Prng* rng)
{
    u64 a = Prng_u64(rng);
    u64 b = Prng_u64(rng);
    a = a ? a : 1;
    b = b ? b : 1;
    return (Guid) { a, b };
}

void guid_fmt(char* dst, i32 size, Guid value)
{
    u32 a0 = (value.a >> 32) & 0xffffffff;
    u32 a1 = (value.a >> 0) & 0xffffffff;
    u32 b0 = (value.b >> 32) & 0xffffffff;
    u32 b1 = (value.b >> 0) & 0xffffffff;
    StrCatf(dst, size, "%x_%x_%x_%x", a0, a1, b0, b1);
}

u32 guid_hashof(Guid x)
{
    return Fnv32Qword(x.b, Fnv32Qword(x.a, Fnv32Bias));
}
