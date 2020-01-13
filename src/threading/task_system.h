#pragma once

#include "common/int_types.h"

enum TaskPriority : u32
{
    TaskPriority_High = 0,
    TaskPriority_Med,
    TaskPriority_Low,

    TaskPriority_COUNT
};

struct ITask
{
    ITask() :
        m_priority(TaskPriority_Med),
        m_taskSize(1),
        m_granularity(1),
        m_iExec(0),
        m_iWait(0)
    {}

    ITask(u32 size, u32 granularity, TaskPriority priority) :
        m_priority(priority),
        m_taskSize(size),
        m_granularity(granularity),
        m_iExec(0),
        m_iWait(0)
    {}

    virtual ~ITask() {}

    bool IsComplete() const;

    virtual void Execute(u32 begin, u32 end, u32 tid) = 0;

    // ------------------------------------------------------------------------

    TaskPriority m_priority;
    u32 m_taskSize;
    u32 m_granularity;

    // ------------------------------------------------------------------------
    i32 m_iExec;
    i32 m_iWait;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Await(ITask* pTask, TaskPriority lowestToRun);
    void AwaitAll();
};
