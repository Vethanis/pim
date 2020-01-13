#pragma once

#include "threading/task_system.h"
#include "containers/array.h"

struct TaskGroup final : ITask
{
    Array<ITask*> m_tasks;

    TaskGroup() : ITask()
    {
        m_tasks.Init(Alloc_Pool);
    }
    TaskGroup(TaskPriority priority)
        : ITask(1, 1, priority)
    {
        m_tasks.Init(Alloc_Pool);
    }
    ~TaskGroup()
    {
        m_tasks.Reset();
    }

    TaskGroup(const TaskGroup& ro) = delete;
    TaskGroup& operator=(const TaskGroup& ro) = delete;

    TaskGroup(TaskGroup&& tmp)
    {
        memcpy(this, &tmp, sizeof(*this));
        memset(&tmp, 0, sizeof(*this));
    }
    TaskGroup& operator=(TaskGroup&& tmp)
    {
        m_tasks.Reset();
        memcpy(this, &tmp, sizeof(*this));
        memset(&tmp, 0, sizeof(*this));
        return *this;
    }

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
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Submit(pTask);
        }
        TaskSystem::Submit(this);
    }

    void Await()
    {
        TaskSystem::Await(this, m_priority);
    }

    void Execute(u32 begin, u32 end, u32 tid) final
    {
        for (ITask* pTask : m_tasks)
        {
            TaskSystem::Await(pTask, m_priority);
        }
        m_tasks.Reset();
    }
};
