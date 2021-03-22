#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/tonemap.h"

PIM_C_BEGIN

typedef struct FrameBuf_s FrameBuf;

void ResolveTile(FrameBuf* target, TonemapId tonemapper, float4 toneParams);

PIM_C_END
