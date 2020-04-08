#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef pim_alignas(16) struct int4_s
{
    i32 x;
    i32 y;
    i32 z;
    i32 w;
} int4;

PIM_C_END
