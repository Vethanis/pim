#include "threading/taskgraph.hpp"
#include "common/atomics.h"
#include "common/console.h"

void TaskGraphImpl::Init()
{
    m_lookup.Clear();
    m_tasks.Clear();
    m_work.Clear();
    m_threads.Clear();
    store_i32(&m_running, 1, MO_Release);
    event_create(&m_beginEvent);
    const i32 tcount = thread_hardware_count();
    barrier_new(&m_finishBarrier, tcount);
    m_threads.Resize(tcount - 1);
    for (thread_t& t : m_threads)
    {
        thread_create(&t, StaticThreadMethod, this);
    }
}

void TaskGraphImpl::Shutdown()
{
    store_i32(&m_running, 0, MO_Release);
    event_wakeall(&m_beginEvent);
    for (thread_t& t : m_threads)
    {
        thread_join(&t);
    }
    barrier_del(&m_finishBarrier);
    event_destroy(&m_beginEvent);
    for (Task& task : m_tasks)
    {
        event_destroy(&task.depEvent);
    }
    m_lookup.Reset();
    m_tasks.Reset();
    m_work.Reset();
    m_threads.Reset();
}

bool TaskGraphImpl::Register(TaskGenerator* generator)
{
    ASSERT(generator);
    ASSERT(generator->GetName().IsValid());
    if (m_lookup.AddCopy(generator->GetName(), m_tasks.Size()))
    {
        Task& task = m_tasks.Grow();
        task = {};
        task.generator = generator;
        event_create(&task.depEvent);
        return true;
    }
    return false;
}

void TaskGraphImpl::Update()
{
    const i32 taskCount = m_tasks.Size();
    for (i32 iTask = 0; iTask < taskCount; ++iTask)
    {
        Task& task = m_tasks[iTask];
        ASSERT(task.generator);
        task.predNames.Clear();
        task.predIndices.Clear();
        task.status = NodeStatus_Init;
        task.worksize = 0;
        task.head = 0;
        task.tail = 0;
        task.visited = 0;
        task.predtally = 0;
        task.node = task.generator->Generate(task.predNames, task.worksize);
        ASSERT(task.worksize >= 0);
    }

    for (Task& task : m_tasks)
    {
        for (TaskName predName : task.predNames)
        {
            ASSERT(predName.IsValid());
            i32* pPredIndex = m_lookup.Get(predName);
            ASSERT(pPredIndex);
            if (pPredIndex)
            {
                i32 iPred = *pPredIndex;
                task.predIndices.Grow() = iPred;
                ASSERT(m_tasks[iPred].node);
            }
        }
    }

    m_work.Clear();
    m_work.Reserve(taskCount);
    for (i32 iTask = 0; iTask < taskCount; ++iTask)
    {
        Task& task = m_tasks[iTask];
        TaskNode* node = task.node;
        if (node && !task.visited)
        {
            Sort_Visit(iTask);
        }
    }

    if (m_work.Size())
    {
        event_wakeall(&m_beginEvent);
        TaskNodeSet preds(EAlloc_Temp);
        ExecuteGraph(preds);
        m_work.Clear();
    }
}

void TaskGraphImpl::Sort_Visit(i32 iTask)
{
    Task& task = m_tasks[iTask];
    ASSERT(task.node);
    ASSERT(task.visited != 1); // cyclic
    if (!task.visited)
    {
        task.visited = 1;
        task.status = 0;
        task.head = 0;
        task.tail = 0;
        task.predtally = task.predIndices.Size();
        if (task.predtally)
        {
            for (TaskIndex iPred : task.predIndices)
            {
                Sort_Visit(iPred);
            }
        }
        m_work.Grow() = iTask;
        task.visited = 2;
    }
}

void TaskGraphImpl::AwaitPreds(TaskIndex iTask, TaskNodeSet& preds)
{
    preds.Clear();
    Task& task = m_tasks[iTask];
    const i32 predCount = task.predIndices.Size();
    if (predCount > 0)
    {
        i32 numComplete = 0;
        for (TaskIndex iPred : task.predIndices)
        {
            Task& pred = m_tasks[iPred];
            preds.Grow() = pred.node;
            if (load_i32(&pred.status, MO_Acquire) == NodeStatus_Complete)
            {
                ++numComplete;
            }
        }
        if (numComplete == predCount)
        {
            store_i32(&task.predtally, 0, MO_Release);
        }
        if (load_i32(&task.predtally, MO_Acquire) == 0)
        {
            event_wakeall(&task.depEvent);
        }
        else
        {
            while (load_i32(&task.predtally, MO_Acquire) != 0)
            {
                event_wait(&task.depEvent);
            }
        }
    }
}

void TaskGraphImpl::ExecuteTaskNode(TaskIndex iTask, TaskNodeSet& preds)
{
    Task& task = m_tasks[iTask];
    TaskNode *const node = task.node;
    ASSERT(node);

    const i32 tasksplit = m_threads.Size() * m_threads.Size();
    const i32 worksize = task.worksize;
    i32 gran = worksize / tasksplit;
    gran = pim_max(1, gran);

    i32 a = fetch_add_i32(&task.head, gran, MO_Acquire);
    i32 b = pim_min(a + gran, worksize);
    while (a < b)
    {
        node->Execute(a, b, preds);

        i32 count = b - a;
        i32 prev = fetch_add_i32(&task.tail, count, MO_Release);
        ASSERT(prev < worksize);
        if ((prev + count) >= worksize)
        {
            store_i32(&task.status, NodeStatus_Complete, MO_Release);
        }
        a = fetch_add_i32(&task.head, gran, MO_Acquire);
        b = pim_min(a + gran, worksize);
    }
}

void TaskGraphImpl::ExecuteGraph(TaskNodeSet& preds)
{
    if (load_i32(&m_running, MO_Acquire))
    {
        preds.Reserve(16);
        for (TaskIndex iTask : m_work)
        {
            AwaitPreds(iTask, preds);
            ExecuteTaskNode(iTask, preds);
        }
        barrier_wait(&m_finishBarrier);
    }
}

void TaskGraphImpl::ThreadMethod()
{
    TaskNodeSet preds;
    while (load_i32(&m_running, MO_Acquire))
    {
        event_wait(&m_beginEvent);
        ExecuteGraph(preds);
    }
}

i32 PIM_CDECL TaskGraphImpl::StaticThreadMethod(void* arg)
{
    TaskGraphImpl* self = reinterpret_cast<TaskGraphImpl*>(arg);
    self->ThreadMethod();
    return 0;
}
