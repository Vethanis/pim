#pragma once

#include "common/unknown.hpp"

class TaskGraphImpl;
class TaskGenerator;

class TaskNode : Unknown
{
public:
    virtual ~TaskNode() {}
    virtual void Execute(i32 a, i32 b, const Array<TaskNode*>& predecessors) = 0;
    virtual Unknown* GetOutput() = 0;
private:
    TaskNode(const TaskNode& other) = delete;
    TaskNode& operator=(const TaskNode& other) = delete;
};
