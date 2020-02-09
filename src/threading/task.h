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
    ITask() : m_state(0), m_waits(0) {}
    virtual ~ITask() {}
    virtual void Execute() = 0;

    TaskState GetState() const;
    bool IsComplete() const;
    bool IsInProgress() const;
    bool IsInitOrComplete() const;

private:
    friend struct ITaskFriend;
    u32 m_state;
    i32 m_waits;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask);
};
