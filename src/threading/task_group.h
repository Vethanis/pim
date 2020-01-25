#pragma once

#include "threading/task_system.h"
#include "containers/array.h"

struct TaskGroup final : ITask
{
    ITask* m_dep;
    Array<ITask*> m_tasks;

    TaskGroup(ITask* dep = nullptr) : ITask(), m_dep(dep)
    {
        m_tasks.Init(Alloc_Pool);
    }
    ~TaskGroup() final
    {
        m_tasks.Reset();
    }
    TaskGroup(const TaskGroup& ro) = delete;
    TaskGroup& operator=(const TaskGroup& ro) = delete;

    void Clear()
    {
        m_tasks.Clear();
    }

    void Add(ITask* pTask)
    {
        ASSERT(pTask);
        m_tasks.PushBack(pTask);
    }

    void Execute(u32 tid) final
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Submit(pTask);
        }
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Await(pTask);
        }
    }
};

template<i32 kPartitions>
struct TaskFor : ITask
{
    virtual void ExecuteFor(i32 begin, i32 end, u32 tid) = 0;

    struct Subtask : ITask
    {
        TaskFor* m_parent;
        i32 m_begin;
        i32 m_end;

        void Execute(u32 tid) final
        {
            TaskFor* parent = m_parent;
            parent->ExecuteFor(m_begin, m_end, tid);
        }
    };

    Subtask m_inner[kPartitions];
    i32 m_count;

    void Execute(u32 tid) final
    {
        const i32 count = m_count;
        const i32 dt = Max(1, count / kPartitions);
        i32 t = 0;
        i32 i = 0;
        while ((t < count) && (i < kPartitions))
        {
            m_inner[i].m_parent = this;
            m_inner[i].m_begin = t;
            m_inner[i].m_end = Min(t + dt, count);
            TaskSystem::Submit(m_inner + i);
            t += dt;
            ++i;
        }
        for (i32 j = 0; j < i; ++j)
        {
            TaskSystem::Await(m_inner + j);
        }
    }
};
