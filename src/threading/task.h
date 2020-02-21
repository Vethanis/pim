#pragma once

#include "common/int_types.h"
#include "containers/array.h"

static constexpr u32 kNumThreads = 16;

u32 ThreadId();
u32 NumActiveThreads();

struct ITask
{
    ITask(i32 begin, i32 end) :
        m_waits(0),
        m_exec(0),
        m_begin(begin),
        m_end(end) {}
    virtual ~ITask() {}
    virtual void Execute(i32 begin, i32 end) = 0;

    bool IsComplete() const;
    bool IsInProgress() const;
    void SetRange(i32 begin, i32 end);

private:
    friend struct ITaskFriend;
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
