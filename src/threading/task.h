#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "os/atomics.h"

struct Task;

namespace TaskSystem
{
    void Submit(Task* pTask);
    void Await(Task* pTask);
};

enum TaskState : u32
{
    TaskState_Init = 0,
    TaskState_Submit,
    TaskState_Execute,
    TaskState_Complete,
};

struct Task
{
    using TaskFn = void(*)(Task& task);

    u32 m_state;
    i32 m_waits;
    TaskFn m_fn;

    TaskState GetState() const { return (TaskState)Load(m_state); }
    bool IsComplete() const { return GetState() == TaskState_Complete; }
    bool IsInProgress() const
    {
        switch (GetState())
        {
        case TaskState_Submit:
        case TaskState_Execute:
            return true;
        }
        return false;
    }
    bool IsInitOrComplete() const
    {
        switch (GetState())
        {
        case TaskState_Init:
        case TaskState_Complete:
            return true;
        }
        return false;
    }

    void Init(TaskFn fn)
    {
        m_state = 0;
        m_waits = 0;
        m_fn = fn;
    }

    void Execute()
    {
        ASSERT(m_fn);
        m_fn(*this);
    }

    void Submit() { TaskSystem::Submit(this); }
    void Await() { TaskSystem::Await(this); }

    template<typename T>
    void InitAs()
    {
        Init(ExecuteAs<T>);
    }

    template<typename T>
    static void ExecuteAs(Task& task)
    {
        reinterpret_cast<T&>(task).Execute();
    }
};
