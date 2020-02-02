#pragma once

#include "threading/task_system.h"
#include "containers/hash_set.h"
#include "containers/array.h"
#include "components/resources.h"
#include "common/comparator_util.h"

struct TaskNodeId
{
    Guid guid;
};

static constexpr Comparator<TaskNodeId> TaskNodeIdComparator = GuidBasedComparator<TaskNodeId>();
using TaskNodeIds = HashSet<TaskNodeId, TaskNodeIdComparator>;

struct TaskNode : ITask
{
    TaskNodeIds m_depedencies;
    ResourceIds m_reads;
    ResourceIds m_writes;
};

namespace TaskGraph
{
    bool Add(TaskNodeId id, TaskNode* pNode);
    TaskNode* Remove(TaskNodeId id);
    TaskNode* Get(TaskNodeId id);
    TaskNodeId Execute();
    void Await(TaskNodeId id);
    void Clear();
};
