#pragma once

#include "threading/task.h"
#include "containers/array.h"
#include "components/resources.h"

struct TaskNode : ITask
{
    u32 m_graphMark;
    Array<TaskNode*> m_inward; // nodes that run before this, aka inward edges

    TaskNode() : ITask()
    {
        m_graphMark = 0;
        m_inward.Init(Alloc_Pool);
    }

    virtual ~TaskNode()
    {
        m_inward.Reset();
    }
};

namespace TaskGraph
{
    bool Add(TaskNode* pNode);
    bool AddDependency(TaskNode* pFirst, TaskNode* pAfter);
    bool RmDependency(TaskNode* pFirst, TaskNode* pAfter);
};
