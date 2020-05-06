#pragma once

#include "common/macro.h"

PIM_C_BEGIN

struct task_s* VertexStage(struct framebuf_s* target);

u64 Vert_EntsCulled(void);
u64 Vert_EntsDrawn(void);

PIM_C_END
