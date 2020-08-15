#include "common/guid.h"
#include "common/fnv1a.h"
#include "common/random.h"
#include "common/stringutil.h"

i32 guid_find(const guid_t* pim_noalias ptr, i32 count, guid_t key)
{
    ASSERT(ptr);
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

guid_t guid_str(const char* str, u64 seed)
{
    if (str)
    {
        u64 a = Fnv64String(str, seed);
        a = a ? a : 1;
        u64 b = Fnv64String(str, a);
        b = b ? b : 1;
        return (guid_t) { a, b };
    }
    return (guid_t) { 0 };
}

guid_t guid_bytes(const void* ptr, i32 nBytes, u64 seed)
{
    if (ptr && nBytes > 0)
    {
        u64 a = Fnv64Bytes(ptr, nBytes, seed);
        a = a ? a : 1;
        u64 b = Fnv64Bytes(ptr, nBytes, a);
        b = b ? b : 1;
        return (guid_t) { a, b };
    }
    return (guid_t) { 0 };
}

guid_t guid_rand(prng_t* rng)
{
    u64 a = prng_u64(rng);
    u64 b = prng_u64(rng);
    a = a ? a : 1;
    b = b ? b : 1;
    return (guid_t) { a, b };
}

void guid_fmt(char* dst, i32 size, guid_t value)
{
    u32 a0 = (value.a >> 32) & 0xffffffff;
    u32 a1 = (value.a >>  0) & 0xffffffff;
    u32 b0 = (value.b >> 32) & 0xffffffff;
    u32 b1 = (value.b >>  0) & 0xffffffff;
    StrCatf(dst, size, "%x_%x_%x_%x", a0, a1, b0, b1);
}

u32 guid_hashof(guid_t x)
{
    return Fnv32Qword(x.b, Fnv32Qword(x.a, Fnv32Bias));
}
