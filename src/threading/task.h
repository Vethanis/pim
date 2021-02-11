#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,
} TaskStatus;

typedef void(PIM_CDECL *task_execute_fn)(void *const task, i32 begin, i32 end);

typedef struct task_s
{
    task_execute_fn execute;
    i32 status;
    i32 worksize;
    i32 head;
    i32 tail;
} task_t;

i32 task_thread_id(void);
i32 task_thread_ct(void);
i32 task_num_active(void);

void task_submit(void *const task, task_execute_fn execute, i32 worksize);
TaskStatus task_stat(void const *const task);
void task_await(void const *const task);
bool task_poll(void const *const task);

void task_run(void *const task, task_execute_fn fn, i32 worksize);

void task_sys_schedule(void);

void task_sys_init(void);
void task_sys_update(void);
void task_sys_shutdown(void);

PIM_C_END
