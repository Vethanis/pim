#include "threading/task.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/queue.h"
#include "components/system.h"
#include "common/random.h"

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
static Queue<ITask*> ms_queues[kNumThreads];
static OS::Mutex ms_locks[kNumThreads];

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
    static i32 GetBegin(ITask* pTask) { return Load(pTask->m_begin, MO_Relaxed); }
    static i32 GetEnd(ITask* pTask) { return Load(pTask->m_end, MO_Relaxed); }
    static i32 GetLoopLen(ITask* pTask) { return Load(pTask->m_loopLen, MO_Relaxed); }
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

TaskStatus ITask::GetStatus() const { return (TaskStatus)Load(m_status, MO_Relaxed); }
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
    Store(m_head, begin);
    Store(m_tail, begin);
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
    return Load(ms_numThreadsRunning, MO_Relaxed) - Load(ms_numThreadsSleeping, MO_Relaxed);
}

static bool StealWork(ITask* pTask, Range& range)
{
    const i32 len = GetLoopLen(pTask);
    const i32 end = GetEnd(pTask);
    const i32 a = Min(FetchAdd(GetHead(pTask), len, MO_Acquire), end);
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
    const i32 prev = FetchAdd(GetTail(pTask), count, MO_AcqRel);
    ASSERT(prev < end);
    return (prev + count) >= end;
}

static ITask* PopTask(u32 tid)
{
    ITask* pTask = 0;
    OS::LockGuard guard(ms_locks[tid]);
    ms_queues[tid].TryPop(pTask);
    return pTask;
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
                Store(GetStatus(pTask), TaskStatus_Complete);
                while (Load(GetWaits(pTask)) > 0)
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

static void AddTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);
    ASSERT(pTask->IsInitOrComplete());

    Store(GetStatus(pTask), TaskStatus_Exec);
    Store(GetHead(pTask), GetBegin(pTask));
    Store(GetTail(pTask), GetBegin(pTask));

    for (u32 i = 1; i < kNumThreads; ++i)
    {
        OS::LockGuard guard(ms_locks[i]);
        ms_queues[i].Push(pTask);
    }

    ms_waitPush.WakeAll();
}

static void WaitForTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);
    if (pTask->GetStatus() == TaskStatus_Init)
    {
        return;
    }

    i32& waits = GetWaits(pTask);
    Inc(waits, MO_Acquire);
    while (!pTask->IsComplete())
    {
        ms_waitExec.Wait();
    }
    Dec(waits, MO_Release);
}

static void TaskLoop(void* pVoid)
{
    Inc(ms_numThreadsRunning, MO_Acquire);

    Random::Seed();

    ms_tid = (u32)((isize)pVoid);
    const u32 tid = ms_tid;
    ASSERT(tid != 0u);

    while (Load(ms_running, MO_Relaxed))
    {
        if (!TryRunTask(tid))
        {
            Inc(ms_numThreadsSleeping, MO_Acquire);
            ms_waitPush.Wait();
            Dec(ms_numThreadsSleeping, MO_Release);
        }
    }

    Dec(ms_numThreadsRunning, MO_Release);
}

namespace TaskSystem
{
    static void Init()
    {
        ms_waitPush.Open();
        ms_waitExec.Open();
        Store(ms_running, 1u);
        for (u32 t = 1u; t < kNumThreads; ++t)
        {
            ms_locks[t].Open();
            ms_queues[t].Init(Alloc_Perm, 256);
            ms_threads[t].Open(TaskLoop, (void*)((isize)t));
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Store(ms_running, 0u);
        while (Load(ms_numThreadsRunning, MO_Relaxed) > 0)
        {
            ms_waitPush.WakeAll();
            ms_waitExec.WakeAll();
            OS::SwitchThread();
        }
        for (u32 t = 1u; t < kNumThreads; ++t)
        {
            ms_threads[t].Join();
            ms_queues[t].Reset();
            ms_locks[t].Close();
        }
        ms_waitPush.Close();
        ms_waitExec.Close();
    }

    void Submit(ITask* pTask)
    {
        AddTask(pTask, ms_tid);
    }
    void Await(ITask* pTask)
    {
        WaitForTask(pTask, ms_tid);
    }

    static System ms_system{ "TaskSystem", {}, Init, Update, Shutdown };
};
