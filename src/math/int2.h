#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef pim_alignas(8) struct int2_s
{
    i32 x;
    i32 y;
} int2;

PIM_C_END
