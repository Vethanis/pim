#pragma once

#include "common/int_types.h"

enum TaskState : u32
{
    TaskState_Init = 0,
    TaskState_Submit,
    TaskState_Execute,
    TaskState_Complete,
};

struct ITask
{
    u32 m_state;
    i32 m_waits;

    ITask() : m_state(TaskState_Init), m_waits(0) {}
    virtual ~ITask() {}
    ITask(const ITask& other) = delete;
    ITask& operator=(const ITask& other) = delete;

    TaskState GetState() const;
    bool IsComplete() const;
    bool IsInProgress() const;

    virtual void Execute(u32 tid) = 0;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask);
};
