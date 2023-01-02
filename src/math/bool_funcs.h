#pragma once

#include "math/types.h"

PIM_C_BEGIN

pim_inline bool VEC_CALL b2_any(bool2 b)
{
    return (b.x | b.y) != 0;
}

pim_inline bool VEC_CALL b2_all(bool2 b)
{
    return (b.x && b.y);
}

pim_inline bool2 VEC_CALL b2_not(bool2 b)
{
    bool2 vec = { !b.x, !b.y };
    return vec;
}

pim_inline bool VEC_CALL b3_any(bool3 b)
{
    return (b.x | b.y | b.z) != 0;
}

pim_inline bool VEC_CALL b3_all(bool3 b)
{
    return (b.x && b.y && b.z);
}

pim_inline bool3 VEC_CALL b3_not(bool3 b)
{
    bool3 vec = { !b.x, !b.y, !b.z };
    return vec;
}

pim_inline bool VEC_CALL b4_any4(bool4 b)
{
    return (b.x | b.y | b.z | b.w) != 0;
}
pim_inline bool VEC_CALL b4_any3(bool4 b)
{
    return (b.x | b.y | b.z | b.w) != 0;
}

pim_inline bool VEC_CALL b4_all4(bool4 b)
{
    return (b.x && b.y && b.z && b.w);
}
pim_inline bool VEC_CALL b4_all3(bool4 b)
{
    return (b.x && b.y && b.z);
}

pim_inline bool4 VEC_CALL b4_not(bool4 b)
{
    bool4 vec = { !b.x, !b.y, !b.z, !b.w };
    return vec;
}

PIM_C_END
