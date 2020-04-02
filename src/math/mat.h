#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/vec.h"

typedef struct mat_s
{
    vec_t c[4];
} mat_t;

PIM_C_END
