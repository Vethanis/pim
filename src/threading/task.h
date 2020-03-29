#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef enum
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,
} TaskStatus;

typedef void(PIM_CDECL *task_execute_fn)(struct task_s* task, int32_t begin, int32_t end);

typedef struct task_s
{
    task_execute_fn execute;
    int32_t status;
    int32_t awaits;
    int32_t worksize;
    int32_t granularity;
    int32_t head;
    int32_t tail;
} task_t;

SASSERT((sizeof(task_t) & 15) == 0);

int32_t task_thread_id(void);
int32_t task_num_active(void);

void task_submit(task_t* task, task_execute_fn execute, int32_t worksize);
TaskStatus task_stat(task_t* task);
void task_await(task_t* task);

void task_sys_init(void);
void task_sys_update(void);
void task_sys_shutdown(void);

PIM_C_END
