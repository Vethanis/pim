#pragma once

#include "threading/task.h"
#include "containers/array.h"

struct TaskGroup final : ITask
{
    TaskGroup(ITask* dep) : ITask(), m_dep(dep)
    {
        m_group.Init(Alloc_Pool);
    }
    ~TaskGroup()
    {
        m_group.Reset();
    }

    void Clear()
    {
        m_group.Clear();
    }

    void Add(ITask* pTask)
    {
        ASSERT(pTask);
        m_group.PushBack(pTask);
    }

    void Execute() final
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (ITask* pTask : m_group)
        {
            TaskSystem::Submit(pTask);
        }
        for (ITask* pTask : m_group)
        {
            TaskSystem::Await(pTask);
        }
    }

private:
    ITask* m_dep;
    Array<ITask*> m_group;
};

struct IForLoop
{
    IForLoop() {}
    virtual ~IForLoop() {}
    virtual void Execute(i32 i) = 0;
};

struct TaskFor final : ITask
{
    TaskFor(IForLoop* pFor, i32 begin, i32 end) :
        ITask(),
        m_pFor(pFor),
        m_begin(begin),
        m_end(end)
    {
        ASSERT(pFor);
        ASSERT(end >= begin);
    }

    void Execute() final
    {
        IForLoop* pFor = m_pFor;
        for (i32 i = m_begin, e = m_end; i < e; ++i)
        {
            pFor->Execute(i);
        }
    }

private:
    IForLoop* m_pFor;
    i32 m_begin;
    i32 m_end;
};

struct TaskParallelFor final : ITask
{
    ITask* m_dep;
    IForLoop* m_loop;
    Array<TaskFor> m_tasks;

    TaskParallelFor(ITask* dep, IForLoop* pLoop, i32 begin, i32 end, i32 granularity) :
        ITask(), m_dep(dep), m_loop(pLoop)
    {
        ASSERT(pLoop);
        ASSERT(end >= begin);
        ASSERT(granularity > 0);
        m_tasks.Init(Alloc_Pool);
        while (begin < end)
        {
            const i32 next = Min(begin + granularity, end);
            m_tasks.PushBack(TaskFor(pLoop, begin, next));
            begin = next;
        }
    }
    ~TaskParallelFor()
    {
        m_tasks.Reset();
    }

    void Execute() final
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (TaskFor& subtask : m_tasks)
        {
            TaskSystem::Submit(&subtask);
        }
        for (TaskFor& subtask : m_tasks)
        {
            TaskSystem::Await(&subtask);
        }
    }
};
