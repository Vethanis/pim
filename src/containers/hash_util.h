#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "common/fnv1a.h"
#include "common/nextpow2.h"

#define hashutil_tomb_mask (1u << 31u)
#define hashutil_hash_mask (~hashutil_tomb_mask)

static u32 hashutil_filled(u32 hash)
{
    return hash & 1u;
}

static u32 hashutil_tomb(u32 hash)
{
    return (hash >> 31u) & 1u;
}

static u32 hashutil_empty(u32 hash)
{
    return hashutil_filled(~hash);
}

static u32 hashutil_nottomb(u32 hash)
{
    return hashutil_tomb(~hash);
}

static u32 hashutil_empty_or_tomb(u32 hash)
{
    return hashutil_empty(hash) | hashutil_tomb(hash);
}

static u32 hashutil_valid(u32 hash)
{
    return hashutil_filled(hash) & hashutil_nottomb(hash);
}

static u32 hashutil_create_hash(u32 hash)
{
    return (hash | 1u) & hashutil_hash_mask;
}

static u32 hashutil_hash(const void* key, i32 sizeOf)
{
    return hashutil_create_hash(Fnv32Bytes(key, sizeOf, Fnv32Bias));
}

PIM_C_END
