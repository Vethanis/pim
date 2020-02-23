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

static constexpr i32 kTaskSplit = kNumThreads * (kNumThreads - 1);

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
    static i32& GetWaits(ITask* pTask) { return pTask->m_waits; }
    static i32& GetExec(ITask* pTask) { return pTask->m_exec; }
    static i32 GetBegin(ITask* pTask) { return pTask->m_begin; }
    static i32 GetEnd(ITask* pTask) { return pTask->m_end; }
    static i32 GetLoopLen(ITask* pTask) { return pTask->m_loopLen; }
    static i32& GetIndex(ITask* pTask) { return pTask->m_index; }
};

static i32& GetWaits(ITask* pTask) { return ITaskFriend::GetWaits(pTask); }
static i32& GetExec(ITask* pTask) { return ITaskFriend::GetExec(pTask); }
static i32 GetBegin(ITask* pTask) { return ITaskFriend::GetBegin(pTask); }
static i32 GetEnd(ITask* pTask) { return ITaskFriend::GetEnd(pTask); }
static i32 GetLoopLen(ITask* pTask) { return ITaskFriend::GetLoopLen(pTask); }
static i32& GetIndex(ITask* pTask) { return ITaskFriend::GetIndex(pTask); }

// ----------------------------------------------------------------------------

bool ITask::IsComplete() const { return Load(m_exec) == 0; }
bool ITask::IsInProgress() const { return Load(m_exec) > 0; }

void ITask::SetRange(i32 begin, i32 end, i32 loopLen)
{
    ASSERT(IsComplete());
    m_begin = begin;
    m_end = end;
    m_loopLen = loopLen;
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
    const i32 a = FetchAdd(GetIndex(pTask), len);
    const i32 b = Min(a + len, end);
    range.begin = a;
    range.end = b;
    return b > a;
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
        Range range = { 0, 0 };
        while (StealWork(pTask, range))
        {
            pTask->Execute(range.begin, range.end);
        }
        if (Dec(GetExec(pTask), MO_Release) == 1)
        {
            // TODO: instead of using thread exec count, use work range for awaits
            // fetchadd processed item count here then wake waiters
            u64 spins = 0;
            while (Load(GetWaits(pTask), MO_Relaxed) > 0)
            {
                ms_waitExec.WakeAll();
                OS::Spin(++spins);
            }
        }
        return true;
    }
    return false;
}

static void AddTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);
    ASSERT(pTask->IsComplete());
    Store(GetIndex(pTask), GetBegin(pTask));
    for (u32 i = 1; i < kNumThreads; ++i)
    {
        Inc(GetExec(pTask), MO_Acquire);
        OS::LockGuard guard(ms_locks[i]);
        ms_queues[i].Push(pTask);
    }
    ms_waitPush.WakeAll();
}

static void WaitForTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);
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
    struct System final : ISystem
    {
        System() : ISystem("TaskSystem") {}
        void Init() final
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
        void Update() final {}
        void Shutdown() final
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
    };
    static System ms_system;

    void Submit(ITask* pTask)
    {
        AddTask(pTask, ms_tid);
    }
    void Await(ITask* pTask)
    {
        WaitForTask(pTask, ms_tid);
    }
};
