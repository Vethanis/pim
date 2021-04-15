#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct ArenaHdl_s
{
    u32 seqno;
} ArenaHdl;

void ArenaSys_Init(void);
void ArenaSys_Shutdown(void);

bool Arena_Exists(ArenaHdl hdl);
ArenaHdl Arena_Acquire(void);
void Arena_Release(ArenaHdl hdl);
void* Arena_Alloc(ArenaHdl hdl, u32 bytes);

PIM_C_END
