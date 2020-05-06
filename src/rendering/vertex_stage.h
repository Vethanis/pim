#pragma once

#include "common/macro.h"

PIM_C_BEGIN

struct task_s* VertexStage(struct framebuf_s* target);

u64 Vert_EntsCulled(void);
u64 Vert_EntsDrawn(void);
u64 Vert_TrisCulled(void);
u64 Vert_TrisDrawn(void);

PIM_C_END
