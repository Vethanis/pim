#pragma once

#include "common/int_types.h"

enum TaskState : u32
{
    TaskState_Init = 0,
    TaskState_Submit,
    TaskState_Execute,
    TaskState_Complete,
};

enum TaskPriority : u32
{
    TaskPriority_High,
    TaskPriority_Med,
    TaskPriority_Low,

    TaskPriority_COUNT
};

struct ITask
{
    ITask(TaskPriority priority = TaskPriority_Med) : m_state(0), m_waits(0), m_priority(priority) {}
    virtual ~ITask() {}
    virtual void Execute() = 0;

    TaskState GetState() const;
    bool IsComplete() const;
    bool IsInProgress() const;
    bool IsInitOrComplete() const;
    TaskPriority GetPriority() const;

private:
    friend struct ITaskFriend;
    u32 m_state;
    i32 m_waits;
    u32 m_priority;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask);
};
