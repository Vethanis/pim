#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "components/components.h"

typedef struct compflag_s
{
    u32 dwords[((CompId_COUNT + 31) / 32)];
} compflag_t;

compflag_t compflag_create(i32 count, ...);

bool compflag_all(compflag_t has, compflag_t all);
bool compflag_any(compflag_t has, compflag_t any);
bool compflag_none(compflag_t has, compflag_t none);

bool compflag_get(compflag_t flag, compid_t id);
void compflag_set(compflag_t* flag, compid_t id);
void compflag_unset(compflag_t* flag, compid_t id);

bool compflag_eq(compflag_t lhs, compflag_t rhs);

PIM_C_END
