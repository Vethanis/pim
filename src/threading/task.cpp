#include "threading/task.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/ptrqueue.h"
#include "components/system.h"
#include "common/random.h"
#include "common/minmax.h"

enum ThreadState : u32
{
    ThreadState_Running,
    ThreadState_AwaitDependency,
    ThreadState_AwaitWork,
    ThreadState_Stopped
};

struct Range { i32 begin, end; };

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static PtrQueue ms_queues[kNumThreads];

static OS::Event ms_waitPush;
static OS::Event ms_waitExec;

static i32 ms_numThreadsRunning;
static i32 ms_numThreadsSleeping;
static u32 ms_running;

static thread_local u32 ms_tid;

// ----------------------------------------------------------------------------
struct ITaskFriend
{
    static i32& GetStatus(ITask* pTask) { return pTask->m_status; }
    static i32& GetWaits(ITask* pTask) { return pTask->m_waits; }
    static i32 GetBegin(ITask* pTask) { return load_i32(&(pTask->m_begin), MO_Relaxed); }
    static i32 GetEnd(ITask* pTask) { return load_i32(&(pTask->m_end), MO_Relaxed); }
    static i32 GetLoopLen(ITask* pTask) { return load_i32(&(pTask->m_loopLen), MO_Relaxed); }
    static i32& GetHead(ITask* pTask) { return pTask->m_head; }
    static i32& GetTail(ITask* pTask) { return pTask->m_tail; }
    static ITask* GetDependency(ITask* pTask) { return pTask->m_dependency; }
};

static i32& GetStatus(ITask* pTask) { return ITaskFriend::GetStatus(pTask); }
static i32& GetWaits(ITask* pTask) { return ITaskFriend::GetWaits(pTask); }
static i32 GetBegin(ITask* pTask) { return ITaskFriend::GetBegin(pTask); }
static i32 GetEnd(ITask* pTask) { return ITaskFriend::GetEnd(pTask); }
static i32 GetLoopLen(ITask* pTask) { return ITaskFriend::GetLoopLen(pTask); }
static i32& GetHead(ITask* pTask) { return ITaskFriend::GetHead(pTask); }
static i32& GetTail(ITask* pTask) { return ITaskFriend::GetTail(pTask); }
static ITask* GetDependency(ITask* pTask) { return ITaskFriend::GetDependency(pTask); }

// ----------------------------------------------------------------------------

TaskStatus ITask::GetStatus() const { return (TaskStatus)load_i32(&m_status, MO_Relaxed); }
bool ITask::IsComplete() const { return GetStatus() == TaskStatus_Complete; }
bool ITask::IsInitOrComplete() const
{
    TaskStatus status = GetStatus();
    return (status == TaskStatus_Init) || (status == TaskStatus_Complete);
}

void ITask::SetDependency(ITask* pPrev)
{
    ASSERT(IsInitOrComplete());
    m_dependency = pPrev;
}

void ITask::SetRange(i32 begin, i32 end, i32 loopLen)
{
    ASSERT(IsInitOrComplete());
    m_begin = begin;
    m_end = end;
    m_loopLen = loopLen;
    store_i32(&m_head, begin, MO_Release);
    store_i32(&m_tail, begin, MO_Release);
}

void ITask::SetRange(i32 begin, i32 end)
{
    const i32 len = end - begin;
    const i32 loop = Max(1, len / kTaskSplit);
    SetRange(begin, end, loop);
}

// ----------------------------------------------------------------------------

u32 ThreadId()
{
    return ms_tid;
}

u32 NumActiveThreads()
{
    return load_i32(&ms_numThreadsRunning, MO_Relaxed) - load_i32(&ms_numThreadsSleeping, MO_Relaxed);
}

static bool StealWork(ITask* pTask, Range& range)
{
    const i32 len = GetLoopLen(pTask);
    const i32 end = GetEnd(pTask);
    const i32 a = Min(fetch_add_i32(&GetHead(pTask), len, MO_Acquire), end);
    const i32 b = Min(a + len, end);
    range.begin = a;
    range.end = b;
    return b > a;
}

static bool ReportProgress(ITask* pTask, Range range)
{
    const i32 count = range.end - range.begin;
    const i32 end = GetEnd(pTask);
    ASSERT(count > 0);
    const i32 prev = fetch_add_i32(&GetTail(pTask), count, MO_AcqRel);
    ASSERT(prev < end);
    return (prev + count) >= end;
}

static ITask* PopTask(u32 tid)
{
    return static_cast<ITask*>(ms_queues[tid].TryPop());
}

static bool TryRunTask(u32 tid)
{
    ITask* pTask = PopTask(tid);
    if (pTask)
    {
        ITask* pDependency = GetDependency(pTask);
        if (pDependency)
        {
            TaskSystem::Await(pDependency);
        }
        Range range = { 0, 0 };
        while (StealWork(pTask, range))
        {
            pTask->Execute(range.begin, range.end);
            if (ReportProgress(pTask, range))
            {
                store_i32(&GetStatus(pTask), TaskStatus_Complete, MO_Release);
                while (load_i32(&GetWaits(pTask), MO_Acquire) > 0)
                {
                    ms_waitExec.WakeAll();
                    OS::SwitchThread();
                }
                break;
            }
        }
        return true;
    }
    return false;
}

static void AddTask(ITask* pTask, u32)
{
    ASSERT(pTask);
    ASSERT(pTask->IsInitOrComplete());

    store_i32(&GetStatus(pTask), TaskStatus_Exec, MO_Release);
    store_i32(&GetHead(pTask), GetBegin(pTask), MO_Release);
    store_i32(&GetTail(pTask), GetBegin(pTask), MO_Release);

    for (u32 i = 1; i < kNumThreads; ++i)
    {
        ms_queues[i].Push(pTask);
    }

    ms_waitPush.WakeAll();
}

static void AddTasks(Slice<ITask*> tasks, u32)
{
    for (ITask* pTask : tasks)
    {
        ASSERT(pTask);
        ASSERT(pTask->IsInitOrComplete());

        store_i32(&GetStatus(pTask), TaskStatus_Exec, MO_Release);
        store_i32(&GetHead(pTask), GetBegin(pTask), MO_Release);
        store_i32(&GetTail(pTask), GetBegin(pTask), MO_Release);
    }

    for (u32 i = 1; i < kNumThreads; ++i)
    {
        for (ITask* pTask : tasks)
        {
            ms_queues[i].Push(pTask);
        }
    }

    ms_waitPush.WakeAll();
}

static void WaitForTask(ITask* pTask, u32)
{
    ASSERT(pTask);
    if (pTask->GetStatus() == TaskStatus_Init)
    {
        return;
    }

    i32& waits = GetWaits(pTask);
    inc_i32(&waits, MO_Acquire);
    while (!pTask->IsComplete())
    {
        ms_waitExec.Wait();
    }
    dec_i32(&waits, MO_Release);
}

static void TaskLoop(void* pVoid)
{
    inc_i32(&ms_numThreadsRunning, MO_Acquire);

    Random::Seed();

    ms_tid = (u32)((isize)pVoid);
    const u32 tid = ms_tid;
    ASSERT(tid != 0u);

    while (load_u32(&ms_running, MO_Relaxed))
    {
        if (!TryRunTask(tid))
        {
            inc_i32(&ms_numThreadsSleeping, MO_Acquire);
            ms_waitPush.Wait();
            dec_i32(&ms_numThreadsSleeping, MO_Release);
        }
    }

    dec_i32(&ms_numThreadsRunning, MO_Release);
}

namespace TaskSystem
{
    static void Init()
    {
        ms_waitPush.Open();
        ms_waitExec.Open();
        store_u32(&ms_running, 1u, MO_Release);
        for (u32 t = 1u; t < kNumThreads; ++t)
        {
            ms_queues[t].Init(Alloc_Perm, 256);
            ms_threads[t].Open(TaskLoop, (void*)((isize)t));
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        store_u32(&ms_running, 0u, MO_Release);
        while (load_i32(&ms_numThreadsRunning, MO_Acquire) > 0)
        {
            ms_waitPush.WakeAll();
            ms_waitExec.WakeAll();
            OS::SwitchThread();
        }
        for (u32 t = 1u; t < kNumThreads; ++t)
        {
            ms_threads[t].Join();
            ms_queues[t].Reset();
        }
        ms_waitPush.Close();
        ms_waitExec.Close();
    }

    void Submit(ITask* pTask)
    {
        AddTask(pTask, ms_tid);
    }

    void Submit(Slice<ITask*> tasks)
    {
        AddTasks(tasks, ms_tid);
    }

    void Await(ITask* pTask)
    {
        WaitForTask(pTask, ms_tid);
    }

    static System ms_system{ "TaskSystem", {}, Init, Update, Shutdown };
};
