#include "components/compflag.h"
#include "common/valist.h"

static const i32 kDwordCount = sizeof(compflag_t) / sizeof(u32);

compflag_t compflag_create(i32 count, ...)
{
    compflag_t result;
    for (i32 i = 0; i < kDwordCount; ++i)
    {
        result.dwords[i] = 0;
    }
    VaList va = VA_START(count);
    for (i32 i = 0; i < count; ++i)
    {
        compid_t id = VA_ARG(va, compid_t);
        compflag_set(&result, id);
    }
    return result;
}

bool compflag_all(compflag_t has, compflag_t all)
{
    u32 test = 0;
    for (i32 i = 0; i < kDwordCount; ++i)
    {
        test |= (has.dwords[i] & all.dwords[i]) - all.dwords[i];
    }
    return !test;
}

bool compflag_any(compflag_t has, compflag_t any)
{
    u32 test = 0;
    for (i32 i = 0; i < kDwordCount; ++i)
    {
        test |= has.dwords[i] & any.dwords[i];
    }
    return test;
}

bool compflag_none(compflag_t has, compflag_t none)
{
    return !compflag_any(has, none);
}

bool compflag_get(compflag_t flag, compid_t id)
{
    u32 dword = id >> 5;
    u32 bit = id & 31;
    u32 mask = 1u << bit;
    return flag.dwords[dword] & mask;
}

void compflag_set(compflag_t* flag, compid_t id)
{
    u32 dword = id >> 5;
    u32 bit = id & 31;
    u32 mask = 1u << bit;
    flag->dwords[dword] |= mask;
}

void compflag_unset(compflag_t* flag, compid_t id)
{
    u32 dword = id >> 5;
    u32 bit = id & 31;
    u32 mask = 1u << bit;
    flag->dwords[dword] &= ~mask;
}

bool compflag_eq(compflag_t lhs, compflag_t rhs)
{
    u32 cmp = 0;
    for (i32 i = 0; i < kDwordCount; ++i)
    {
        cmp |= lhs.dwords[i] - rhs.dwords[i];
    }
    return cmp == 0;
}
