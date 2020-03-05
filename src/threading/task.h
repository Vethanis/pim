#pragma once

#include "common/int_types.h"
#include "containers/slice.h"

static constexpr u32 kNumThreads = 32;
static constexpr i32 kTaskSplit = kNumThreads * (kNumThreads / 2);

u32 ThreadId();
u32 NumActiveThreads();

enum TaskStatus : i32
{
    TaskStatus_Init = 0,
    TaskStatus_Exec,
    TaskStatus_Complete,

    TaskStatus_COUNT
};

struct ITask
{
    ITask(i32 begin, i32 end, i32 loopLen) :
        m_dependency(0),
        m_status(TaskStatus_Init),
        m_waits(0),
        m_begin(begin),
        m_end(end),
        m_loopLen(loopLen),
        m_head(begin),
        m_tail(begin)
    {}
    virtual ~ITask() {}
    virtual void Execute(i32 begin, i32 end) = 0;

    TaskStatus GetStatus() const;
    bool IsComplete() const;
    bool IsInitOrComplete() const;
    void SetDependency(ITask* pTask);
    void SetRange(i32 begin, i32 end, i32 loopLen);
    void SetRange(i32 begin, i32 end);

private:
    friend struct ITaskFriend;
    ITask* m_dependency;
    i32 m_status;
    i32 m_waits;
    i32 m_begin;
    i32 m_end;
    i32 m_loopLen;
    i32 m_head;
    i32 m_tail;
};

namespace TaskSystem
{
    void Submit(ITask* pTask);
    void Submit(Slice<ITask*> tasks);
    void Await(ITask* pTask);
};
