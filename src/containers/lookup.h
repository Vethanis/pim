#pragma once

#include "common/macro.h"
#include "common/guid.h"

typedef struct Lookup_s
{
    i32* pim_noalias indices;
    u32 width;
} Lookup;

void Lookup_New(Lookup *const loo);
void Lookup_Del(Lookup *const loo);

void Lookup_Reserve(
    Lookup *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    i32 capacity);

i32 Lookup_Insert(
    Lookup *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    Guid key,
    i32 keyIndex);

bool Lookup_Find(
    Lookup const *const loo,
    const Guid* pim_noalias names,
    i32 nameCount,
    Guid key,
    i32 *const pim_noalias nameIndexOut,
    i32 *const pim_noalias iLookupOut);

void Lookup_Remove(
    Lookup *const loo,
    i32 keyIndex,
    i32 iLookup);

void Lookup_Clear(Lookup *const loo);
