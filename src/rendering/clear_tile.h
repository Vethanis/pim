#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct framebuf_s framebuf_t;
typedef struct task_s task_t;

task_t* ClearTile(framebuf_t* target, float4 color, float depth);

PIM_C_END
