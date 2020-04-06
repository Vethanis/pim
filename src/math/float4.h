#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef pim_alignas(16) struct float4_s
{
    float x;
    float y;
    float z;
    float w;
} float4;

PIM_C_END
