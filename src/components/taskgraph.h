#pragma once

#include "threading/task.h"

struct TaskId
{
    i32 value;
    i32 frame;
};

namespace TaskGraph
{
    TaskId Add(Task* pTask);
    bool AddDependency(TaskId src, TaskId dst);
};
