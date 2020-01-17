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
    ITask() :
        m_state(TaskState_Init),
        m_iWait(0)
    {}
    virtual ~ITask() {}
    ITask(const ITask& other) = delete;
    ITask& operator=(const ITask& other) = delete;

    bool IsComplete() const;
    TaskState GetState() const;
    void AwaitIfQueued();
    void SubmitIfNotQueued();

    virtual void Execute(u32 tid) = 0;

    // ------------------------------------------------------------------------
    u32 m_state;
    i32 m_iWait;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask);
    void AwaitAll();
};
