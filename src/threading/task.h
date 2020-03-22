#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef enum
{
    TaskStatus_Exec = 0,
    TaskStatus_Complete,
} TaskStatus;

typedef void (*task_execute_fn)(void* userData, int32_t begin, int32_t end);
typedef void* task_hdl;

int32_t task_thread_id(void);
int32_t task_num_active(void);

task_hdl task_submit(void* data, task_execute_fn execute, int32_t worksize);
TaskStatus task_stat(task_hdl hdl);
void* task_complete(task_hdl* pHandle);

void task_sys_init(void);
void task_sys_update(void);
void task_sys_shutdown(void);

PIM_C_END
