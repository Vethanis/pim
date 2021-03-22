#pragma once

#include "common/macro.h"
#include "containers/queue_i32.h"
#include "containers/genid.h"

PIM_C_BEGIN

typedef struct IdAlloc_s
{
    i32 length;
    u8* pim_noalias versions;
    IntQueue freelist;
} IdAlloc;

void IdAlloc_New(IdAlloc* ia);
void IdAlloc_Del(IdAlloc* ia);
void IdAlloc_Clear(IdAlloc* ia);

i32 IdAlloc_Capacity(const IdAlloc* ia);
i32 IdAlloc_Size(const IdAlloc* ia);

bool IdAlloc_Exists(const IdAlloc* ia, GenId id);
bool IdAlloc_ExistsAt(const IdAlloc* ia, i32 index);

GenId IdAlloc_Alloc(IdAlloc* ia);

bool IdAlloc_Free(IdAlloc* ia, GenId id);
bool IdAlloc_FreeAt(IdAlloc* ia, i32 index);

PIM_C_END
