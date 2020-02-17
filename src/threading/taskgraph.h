#pragma once

#include "threading/task.h"
#include "containers/array.h"

struct TaskNode : ITask
{
    TaskNode() : ITask(0, 0), m_graphId(-1) {}
    virtual void BeforeSubmit() {}

    i32 m_graphId;
};

namespace TaskGraph
{
    void AddVertex(TaskNode* pNode);
    void AddVertices(Slice<TaskNode*> nodes);
    bool AddEdge(TaskNode* src, TaskNode* dst);
    void Evaluate();
};
