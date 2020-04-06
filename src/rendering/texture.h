#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct texture_s
{
    int32_t width;
    int32_t height;
    uint32_t* texels;
} texture_t;

PIM_C_END
