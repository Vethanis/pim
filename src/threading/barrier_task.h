#pragma once

#include "threading/task.h"

// Introduces a thread barrier into the task pipeline,
// where tasks before the barrier must be completed before tasks after the barrier may begin.
struct BarrierTask final : ITask
{
    BarrierTask() : ITask(0, kNumThreads * 2, 1), m_await(nullptr) {}
    void Setup(ITask* pTask)
    {
        m_await = pTask;
        SetRange(0, kNumThreads * 2, 1);
    }
    void Execute(i32, i32) final
    {
        if (m_await)
        {
            // Since each thread queue sees every task and steals work,
            // an await will ensure all threads are done with prior tasks before starting.
            TaskSystem::Await(m_await);
        }
    }
private:
    ITask* m_await;
};
