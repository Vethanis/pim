#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,
} TaskStatus;

typedef void(PIM_CDECL *TaskExecuteFn)(void* task, i32 begin, i32 end);

typedef struct Task_s
{
    TaskExecuteFn execute;
    i32 status;
    i32 worksize;
    i32 head;
    i32 tail;
} Task;

i32 Task_ThreadId(void);
i32 Task_ThreadCount(void);

void Task_Submit(void* task, TaskExecuteFn execute, i32 worksize);
TaskStatus Task_Stat(const void* task);
void Task_Await(void* task);

void Task_Run(void* task, TaskExecuteFn fn, i32 worksize);

void TaskSys_Schedule(void);

void TaskSys_Init(void);
void TaskSys_Update(void);
void TaskSys_Shutdown(void);

PIM_C_END
