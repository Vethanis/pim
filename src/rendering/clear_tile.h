#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

struct task_s* ClearTile(struct framebuf_s* target, float4 color, float depth);

PIM_C_END
