#pragma once

#include "common/int_types.h"

enum TaskState : u32
{
    TaskState_Init = 0,
    TaskState_Execute,
    TaskState_Complete,
};

struct ITask
{
    ITask(i32 begin, i32 end) :
        m_state(0),
        m_waits(0),
        m_exec(0),
        m_begin(begin),
        m_end(end)
    {}
    virtual ~ITask() {}
    virtual void Execute(i32 begin, i32 end) = 0;

    TaskState GetState() const;
    bool IsComplete() const;
    bool IsInProgress() const;
    bool IsInitOrComplete() const;

private:
    friend struct ITaskFriend;
    u32 m_state;
    i32 m_waits;
    i32 m_exec;
    i32 m_begin;
    i32 m_end;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask);
};
