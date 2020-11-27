#pragma once

#include "common/macro.h"

typedef struct GenId
{
    u32 version : 8;
    u32 index : 24;
} GenId;
