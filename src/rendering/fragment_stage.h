#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;

void drawables_fragment(framebuf_t* frontBuf, const framebuf_t* backBuf);

PIM_C_END
