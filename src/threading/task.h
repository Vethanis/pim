#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,
} TaskStatus;

typedef void(PIM_CDECL *task_execute_fn)(void* task, i32 begin, i32 end);

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

void task_submit(void* task, task_execute_fn execute, i32 worksize);
TaskStatus task_stat(const void* task);
void task_await(void* task);

void task_run(void* task, task_execute_fn fn, i32 worksize);

void task_sys_schedule(void);

void task_sys_init(void);
void task_sys_update(void);
void task_sys_shutdown(void);

PIM_C_END
