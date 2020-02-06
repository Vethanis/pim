#pragma once

#include "threading/task.h"
#include "containers/array.h"

struct TaskGroup
{
    Task m_base;
    Task* m_dep;
    Array<Task*> m_group;

    void Init(Task* dep = nullptr)
    {
        m_base.InitAs<TaskGroup>();
        m_dep = dep;
        m_group.Init(Alloc_Pool);
    }

    void Reset()
    {
        m_group.Reset();
    }

    void Clear()
    {
        m_group.Clear();
    }

    void Add(Task* pTask)
    {
        ASSERT(pTask);
        m_group.PushBack(pTask);
    }

    void Submit() { m_base.Submit(); }
    void Await() { m_base.Await(); }

    void Execute()
    {
        if (m_dep)
        {
            TaskSystem::Await(m_dep);
        }
        for (Task* pTask : m_group)
        {
            TaskSystem::Submit(pTask);
        }
        for (Task* pTask : m_group)
        {
            TaskSystem::Await(pTask);
        }
    }
};

template<typename TTask>
struct TaskParallel
{
    Task m_base;
    Array<TTask> m_tasks;

    void Init()
    {
        m_base.InitAs<TaskParallel>();
        m_tasks.Init(Alloc_Pool);
    }

    void Reset()
    {
        m_tasks.Reset();
    }

    void Clear()
    {
        m_tasks.Clear();
    }

    void Add(const TTask& task)
    {
        m_tasks.PushBack(task);
    }

    void Submit() { m_base.Submit(); }
    void Await() { m_base.Await(); }

    void Execute()
    {
        for (TTask& task : m_tasks)
        {
            reinterpret_cast<Task&>(task).Submit();
        }
        for (TTask& task : m_tasks)
        {
            reinterpret_cast<Task&>(task).Await();
        }
    }
};

struct ForLoop
{
    using ForFn = void(*)(ForLoop& loop, i32 begin, i32 end);

    ForFn m_fn;

    void Execute(i32 begin, i32 end)
    {
        m_fn(*this, begin, end);
    }

    template<typename T>
    void InitAs()
    {
        m_fn = Wrapper<T>;
    }

    template<typename T>
    static void Wrapper(ForLoop& loop, i32 begin, i32 end)
    {
        reinterpret_cast<T&>(loop).Execute(begin, end);
    }
};

template<typename TForLoop>
struct TaskParallelFor
{
    struct Subtask
    {
        Task m_base;
        TForLoop* m_pLoop;
        i32 m_begin;
        i32 m_end;

        void Init(TForLoop* p, i32 b, i32 e)
        {
            m_base.InitAs<Subtask>();
            m_pLoop = p;
            m_begin = b;
            m_end = e;
        }

        void Execute()
        {
            m_pLoop->Execute(m_begin, m_end);
        }

        void Submit() { task.Submit(); }
        void Await() { task.Await(); }
    };

    Task m_base;
    TForLoop m_loop;
    Array<Subtask> m_tasks;

    void Init(const TForLoop& loop, i32 begin, i32 end, i32 granularity = 64)
    {
        ASSERT(end >= begin);
        ASSERT(granularity > 0);
        m_base.InitAs<TaskParallelFor>();
        m_loop = loop;
        m_tasks.Init(Alloc_Pool);
        while (begin < end)
        {
            const i32 next = Min(begin + granularity, end);
            m_tasks.Grow().Init(&m_loop, begin, next);
            begin = next;
        }
    }

    void Reset()
    {
        m_tasks.Reset();
    }

    void Execute()
    {
        for (Subtask& subtask : m_tasks)
        {
            subtask.Submit();
        }
        for (Subtask& subtask : m_tasks)
        {
            subtask.Await();
        }
    }

    void Submit() { m_base.Submit(); }
    void Await() { m_base.Await(); }
};

