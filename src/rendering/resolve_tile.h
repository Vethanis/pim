#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "rendering/tonemap.h"

struct task_s* ResolveTile(struct framebuf_s* target, TonemapId tonemapper, float4 toneParams);

PIM_C_END
