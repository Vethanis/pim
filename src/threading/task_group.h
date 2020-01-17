#pragma once

#include "threading/task_system.h"
#include "containers/array.h"

struct TaskGroup final : ITask
{
    ITask* m_dependency;
    Array<ITask*> m_tasks;

    TaskGroup(ITask* dependency)
        : ITask(),
        m_dependency(dependency)
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

    void Submit()
    {
        TaskSystem::Submit(this);
    }

    void Await()
    {
        TaskSystem::Await(this);
    }

    void Execute(u32 tid) final
    {
        if (m_dependency)
        {
            TaskSystem::Await(m_dependency);
        }
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Submit(pTask);
        }
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Await(pTask);
        }
        m_tasks.Reset();
    }
};
