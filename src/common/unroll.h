#pragma once

#include "common/macro.h"

PIM_C_BEGIN

pim_inline void Unroll16(i32 count, i32* n16, i32* n4, i32* n1)
{
    *n16 = count >> 4;
    *n4 = (count >> 2) & 3;
    *n1 = count & 3;
}

pim_inline void Unroll64(i32 count, i32* n64, i32* n8, i32* n1)
{
    *n64 = count >> 6;
    *n8 = (count >> 3) & 7;
    *n1 = count & 7;
}

PIM_C_END
