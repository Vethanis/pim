#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct genid_s
{
    u32 version : 8;
    u32 index : 24;
} GenId;

typedef union { u32 num; GenId id; } genid_u;

PIM_C_END
