#pragma once

#include "common/macro.h"

typedef union GenId
{
    struct
    {
        u32 version : 8;
        u32 index : 24;
    };
    u32 asint;
} GenId;
