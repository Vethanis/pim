#pragma once

#include "threading/task.h"

struct TaskId
{
    i32 value;
    i32 frame;

    bool operator==(TaskId rhs) const
    {
        return ((value - rhs.value) | (frame - rhs.frame)) == 0;
    }
};

namespace TaskGraph
{
    TaskId Add(ITask* pTask);
    bool AddDependency(TaskId src, TaskId dst);
};
