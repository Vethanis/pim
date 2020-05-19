#pragma once

#include "common/macro.h"

PIM_C_BEGIN

struct task_s* Drawables_Vertex(struct tables_s* tables, const struct camera_s* camera);
u64 Vert_EntsCulled(void);
u64 Vert_EntsDrawn(void);

PIM_C_END
