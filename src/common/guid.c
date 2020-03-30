#include "common/guid.h"
#include "common/fnv1a.h"
#include "common/random.h"

int32_t guid_find(const guid_t* ptr, int32_t count, guid_t key)
{
    ASSERT(ptr);
    ASSERT(count >= 0);
    for (int32_t i = 0; i < count; ++i)
    {
        if (guid_eq(ptr[i], key))
        {
            return i;
        }
    }
    return -1;
}

guid_t StrToGuid(const char* str, uint64_t seed)
{
    ASSERT(str);
    uint64_t a = Fnv64String(str, seed);
    a = a ? a : 1;
    uint64_t b = Fnv64String(str, a);
    b = b ? b : 1;
    return (guid_t) { a, b };
}

guid_t BytesToGuid(const void* ptr, int32_t nBytes, uint64_t seed)
{
    ASSERT(ptr);
    ASSERT(nBytes >= 0);
    uint64_t a = Fnv64Bytes(ptr, nBytes, seed);
    a = a ? a : 1;
    uint64_t b = Fnv64Bytes(ptr, nBytes, a);
    b = b ? b : 1;
    return (guid_t) { a, b };
}

guid_t RandGuid()
{
    uint64_t a = rand_int();
    a = (a << 32) | rand_int();
    uint64_t b = rand_int();
    b = (b << 32) | rand_int();
    a = a ? a : 1;
    b = b ? b : 1;
    return (guid_t) { a, b };
}
