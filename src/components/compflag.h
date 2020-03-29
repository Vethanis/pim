#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "components/components.h"

typedef struct compflag_s
{
    uint32_t dwords[((CompId_COUNT + 31) / 32)];
} compflag_t;

compflag_t compflag_create(int32_t count, ...);

int32_t compflag_all(compflag_t has, compflag_t all);
int32_t compflag_any(compflag_t has, compflag_t any);
int32_t compflag_none(compflag_t has, compflag_t none);

int32_t compflag_get(compflag_t flag, compid_t id);
void compflag_set(compflag_t* flag, compid_t id);
void compflag_unset(compflag_t* flag, compid_t id);

PIM_C_END
