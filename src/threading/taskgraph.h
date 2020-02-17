#pragma once

#include "threading/task.h"
#include "containers/array.h"

struct TaskNode : ITask
{
    TaskNode(i32 begin, i32 end) : ITask(begin, end), m_mark(0) { m_edges.Init(Alloc_Tlsf); }
    virtual ~TaskNode() { m_edges.Reset(); }

private:
    friend struct TaskGraphImpl;
    i32 m_mark;
    Array<TaskNode*> m_edges;
};

namespace TaskGraph
{
    void AddVertex(TaskNode* pNode);
    void AddVertices(Slice<TaskNode*> nodes);
    bool AddEdge(TaskNode* src, TaskNode* dst);
    bool RmEdge(TaskNode* src, TaskNode* dst);
    void ClearEdges(TaskNode* dst);
    void Evaluate();
};
