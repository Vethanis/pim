#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "common/fnv1a.h"
#include "common/nextpow2.h"

#define hashutil_tomb_mask (1u << 31u)
#define hashutil_hash_mask (~hashutil_tomb_mask)

pim_inline u32 HashUtil_Filled(u32 hash)
{
    return hash & 1u;
}

pim_inline u32 HashUtil_Tomb(u32 hash)
{
    return (hash >> 31u) & 1u;
}

pim_inline u32 HashUtil_Empty(u32 hash)
{
    return HashUtil_Filled(~hash);
}

pim_inline u32 HashUtil_NotTomb(u32 hash)
{
    return HashUtil_Tomb(~hash);
}

pim_inline u32 HashUtil_EmptyOrTomb(u32 hash)
{
    return HashUtil_Empty(hash) | HashUtil_Tomb(hash);
}

pim_inline u32 HashUtil_Valid(u32 hash)
{
    return HashUtil_Filled(hash) & HashUtil_NotTomb(hash);
}

pim_inline u32 HashUtil_CreateHash(u32 hash)
{
    return (hash | 1u) & hashutil_hash_mask;
}

pim_inline u32 HashUtil_HashBytes(void const *const pim_noalias key, i32 sizeOf)
{
    return HashUtil_CreateHash(Fnv32Bytes(key, sizeOf, Fnv32Bias));
}

PIM_C_END
