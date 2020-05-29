#pragma once

#include "common/macro.h"
#include "components/table.h"
#include "threading/task.h"
#include "rendering/camera.h"
#include "rendering/framebuffer.h"

PIM_C_BEGIN

table_t* Drawables_New(tables_t* tables);
void Drawables_Del(tables_t* tables);
table_t* Drawables_Get(tables_t* tables);

task_t* Drawables_TRS(tables_t* tables);
task_t* Drawables_Bounds(tables_t* tables);
task_t* Drawables_Cull(
    tables_t* tables,
    const camera_t* camera,
    const framebuf_t* backBuf);

PIM_C_END
