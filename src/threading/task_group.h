#pragma once

#include "threading/task.h"
#include "containers/array.h"

struct TaskGroup final : ITask
{
    TaskGroup(ITask* dep) : ITask(), m_dep(dep)
    {
        m_group.Init(Alloc_Tlsf);
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

    void Submit()
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (ITask* pTask : m_group)
        {
            TaskSystem::Submit(pTask);
        }
        TaskSystem::Submit(this);
    }

    void Execute() final
    {
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
    virtual void Execute(i32 begin, i32 end) = 0;
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

    void Execute() final { m_pFor->Execute(m_begin, m_end); }

private:
    IForLoop* m_pFor;
    i32 m_begin;
    i32 m_end;
};

struct TaskParallelFor final : ITask
{
public:
    TaskParallelFor(ITask* dep, IForLoop* pLoop, i32 begin, i32 end, i32 granularity) :
        ITask(), m_dep(dep), m_loop(pLoop)
    {
        ASSERT(pLoop);
        ASSERT(end >= begin);
        ASSERT(granularity > 0);
        m_tasks.Init(Alloc_Tlsf);
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

    void Submit()
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (TaskFor& subtask : m_tasks)
        {
            TaskSystem::Submit(&subtask);
        }
        TaskSystem::Submit(this);
    }

    void Execute() final
    {
        for (TaskFor& subtask : m_tasks)
        {
            TaskSystem::Await(&subtask);
        }
    }

private:
    ITask* m_dep;
    IForLoop* m_loop;
    Array<TaskFor> m_tasks;
};
