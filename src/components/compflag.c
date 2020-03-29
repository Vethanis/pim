#include "components/compflag.h"

#include "common/valist.h"
#include <string.h>

compflag_t compflag_create(int32_t count, ...)
{
    compflag_t result;
    memset(&result, 0, sizeof(result));
    VaList va = VA_START(count);
    for (int32_t i = 0; i < count; ++i)
    {
        compid_t id = VA_ARG(va, compid_t);
        compflag_set(&result, id);
    }
    return result;
}

int32_t compflag_all(compflag_t has, compflag_t all)
{
    uint32_t test = 0;
    const int32_t count = sizeof(compflag_t) / sizeof(uint32_t);
    for (int32_t i = 0; i < count; ++i)
    {
        test |= (has.dwords[i] & all.dwords[i]) - all.dwords[i];
    }
    return !test;
}

int32_t compflag_any(compflag_t has, compflag_t any)
{
    uint32_t test = 0;
    const int32_t count = sizeof(compflag_t) / sizeof(uint32_t);
    for (int32_t i = 0; i < count; ++i)
    {
        test |= has.dwords[i] & any.dwords[i];
    }
    return test;
}

int32_t compflag_none(compflag_t has, compflag_t none)
{
    return !compflag_any(has, none);
}

int32_t compflag_get(compflag_t flag, compid_t id)
{
    uint32_t dword = id >> 5;
    uint32_t bit = id & 31;
    uint32_t mask = 1u << bit;
    return flag.dwords[dword] & mask;
}

void compflag_set(compflag_t* flag, compid_t id)
{
    uint32_t dword = id >> 5;
    uint32_t bit = id & 31;
    uint32_t mask = 1u << bit;
    flag->dwords[dword] |= mask;
}

void compflag_unset(compflag_t* flag, compid_t id)
{
    uint32_t dword = id >> 5;
    uint32_t bit = id & 31;
    uint32_t mask = 1u << bit;
    flag->dwords[dword] &= ~mask;
}
