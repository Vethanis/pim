#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/tonemap.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;

void ResolveTile(framebuf_t* target, TonemapId tonemapper, float4 toneParams);

PIM_C_END
