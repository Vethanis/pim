#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct camera_s camera_t;
typedef struct task_s task_t;

task_t* drawables_vertex(const camera_t* camera);

PIM_C_END
