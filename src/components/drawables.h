#pragma once

#include "common/macro.h"

PIM_C_BEGIN

struct table_s* Drawables_New(struct tables_s* tables);
void Drawables_Del(struct tables_s* tables);
struct table_s* Drawables_Get(struct tables_s* tables);

struct task_s* Drawables_TRS(struct tables_s* tables);
struct task_s* Drawables_Bounds(struct tables_s* tables);
struct task_s* Drawables_Cull(struct tables_s* tables, const struct camera_s* camera, const struct framebuf_s* backBuf);

PIM_C_END
