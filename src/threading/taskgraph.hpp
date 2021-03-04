#pragma once

#include "common/macro.h"
#include "containers/hash32.hpp"
#include "containers/array.hpp"
#include "containers/dict.hpp"
#include "threading/tasknode.hpp"
#include "threading/thread.h"
#include "threading/semaphore.h"
#include "threading/event.h"
#include "threading/barrier.h"

using TaskName = Hash32;
using TaskNameSet = Array<TaskName>;
using TaskNodeSet = Array<TaskNode*>;

class TaskGenerator
{
public:
    TaskGenerator(TaskName name) : m_name(name) {}
    virtual ~TaskGenerator() {}
    virtual TaskNode* Generate(TaskNameSet& predecessorsOut, i32& workSizeOut) = 0;
    TaskName GetName() const { return m_name; }
private:
    TaskName m_name;
};

class TaskGraphImpl final
{
public:
    void Init();
    void Shutdown();
    void Update();

    bool Register(TaskGenerator* generator);

private:

    using TaskIndex = i32;
    using TaskIndexSet = Array<TaskIndex>;

    enum NodeStatus
    {
        NodeStatus_Init = 0,
        NodeStatus_Exec,
        NodeStatus_Complete,
    };
    struct Task
    {
        TaskGenerator* generator;
        TaskNode* node;
        TaskNameSet predNames;
        TaskIndexSet predIndices;
        event_t depEvent;
        i32 status;
        i32 worksize;
        i32 head;
        i32 tail;
        i32 visited;
        i32 predtally;
    };

    void Sort_Visit(TaskIndex iTask);
    void AwaitPreds(TaskIndex iTask, TaskNodeSet& preds);
    void ExecuteTaskNode(TaskIndex iTask, TaskNodeSet& preds);
    void ExecuteGraph(TaskNodeSet& preds);
    void ThreadMethod();

    static i32 PIM_CDECL StaticThreadMethod(void* arg);

    Dict<TaskName, TaskIndex> m_lookup;
    Array<Task> m_tasks;

    Array<TaskIndex> m_work;
    Array<thread_t> m_threads;

    event_t m_beginEvent;
    barrier_t m_finishBarrier;
    i32 m_running;
};

