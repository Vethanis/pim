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
