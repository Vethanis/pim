#include "common/guid.h"
#include "common/fnv1a.h"
#include "common/random.h"

i32 guid_find(const guid_t* ptr, i32 count, guid_t key)
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

guid_t StrToGuid(const char* str, u64 seed)
{
    ASSERT(str);
    u64 a = Fnv64String(str, seed);
    a = a ? a : 1;
    u64 b = Fnv64String(str, a);
    b = b ? b : 1;
    return (guid_t) { a, b };
}

guid_t BytesToGuid(const void* ptr, i32 nBytes, u64 seed)
{
    ASSERT(ptr);
    ASSERT(nBytes >= 0);
    u64 a = Fnv64Bytes(ptr, nBytes, seed);
    a = a ? a : 1;
    u64 b = Fnv64Bytes(ptr, nBytes, a);
    b = b ? b : 1;
    return (guid_t) { a, b };
}

guid_t RandGuid(struct prng_s* rng)
{
    u64 a = prng_u32(rng);
    a = (a << 32) | prng_u32(rng);
    u64 b = prng_u32(rng);
    b = (b << 32) | prng_u32(rng);
    a = a ? a : 1;
    b = b ? b : 1;
    return (guid_t) { a, b };
}
