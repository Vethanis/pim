#include "threading/task_system.h"
#include "os/thread.h"
#include "containers/pipe.h"
#include "components/system.h"
#include "common/random.h"

// ----------------------------------------------------------------------------

struct Subtask
{
    ITask* pTask;
    u32 begin;
    u32 end;
};

enum ThreadState : u32
{
    ThreadState_Running,
    ThreadState_AwaitDependency,
    ThreadState_AwaitWork,
    ThreadState_Stopped
};

static constexpr u32 kCacheLineSize = 64;
static constexpr u64 kMaxSpins = 10;
static constexpr u64 kTicksPerSpin = 100;
static constexpr u32 kNumPriorities = TaskPriority_COUNT;
static constexpr u32 kNumThreads = 16;
static constexpr u32 kPipeDepth = kNumThreads;
static constexpr u32 kThreadMask = kNumThreads - 1u;
static constexpr u32 kNumInitialPartitions = kNumThreads - 1u;
static constexpr u32 kNumPartitions = kNumThreads * kNumInitialPartitions;
static constexpr u32 kTotalCapacity = kPipeDepth * kNumThreads * kNumPriorities;

struct alignas(kCacheLineSize) thread_state_t
{
    u32 state;
    u8 padding[kCacheLineSize - sizeof(u32)];
};

using TaskPipe = ScatterPipe<Subtask, kPipeDepth>;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static thread_state_t ms_states[kNumThreads];
static TaskPipe ms_pipes[kNumThreads][kNumPriorities];

static i32 ms_iWaitingForNew;
static OS::LightSema ms_newTaskSema;

static i32 ms_iWaitingForCompletes;
static OS::LightSema ms_completeTaskSema;

static i32 ms_numThreadsRunning;
static u32 ms_running;

static thread_local u32 ms_tid;

// ----------------------------------------------------------------------------

static u32 PushThreadState(u32 tid, ThreadState state)
{
    u32 prev = Load(ms_states[tid].state, MO_Relaxed);
    Store(ms_states[tid].state, state, MO_SeqCst);
    return prev;
}

static void PopThreadState(u32 tid, u32 prev)
{
    Store(ms_states[tid].state, prev, MO_Release);
}

static u32 RangeToRun(u32 granularity, u32 setSize)
{
    ASSERT(granularity > 0u);
    ASSERT(setSize > 0u);
    return Max(granularity, setSize / kNumPartitions);
}

static u32 RangeToSplit(u32 granularity, u32 setSize)
{
    ASSERT(granularity > 0u);
    ASSERT(setSize > 0u);
    return Max(granularity, setSize / kNumInitialPartitions);
}

static bool IsComplete(const ITask* pTask)
{
    ASSERT(pTask);
    const i32 iExec = Load(pTask->m_iExec, MO_Acquire);
    ASSERT(iExec >= 0);
    return iExec == 0;
}

bool ITask::IsComplete() const { return ::IsComplete(this); }

static bool HasWaits(const ITask* pTask)
{
    ASSERT(pTask);
    const i32 iWait = Load(pTask->m_iWait, MO_Acquire);
    ASSERT(iWait >= 0);
    return iWait > 0;
}

static i32 IncExec(ITask* pTask)
{
    ASSERT(pTask);
    const i32 prev = Inc(pTask->m_iExec, MO_Acquire);
    ASSERT(prev >= 0);
    return prev;
}

static i32 DecExec(ITask* pTask)
{
    ASSERT(pTask);
    const i32 prev = Dec(pTask->m_iExec, MO_Release);
    ASSERT(prev > 0);
    return prev;
}

static i32 IncWait(ITask* pTask)
{
    ASSERT(pTask);
    const i32 prev = Inc(pTask->m_iWait, MO_Acquire);
    ASSERT(prev >= 0);
    return prev;
}

static i32 DecWait(ITask* pTask)
{
    ASSERT(pTask);
    const i32 prev = Dec(pTask->m_iWait, MO_Release);
    ASSERT(prev > 0);
    return prev;
}

static bool HasTasks(u32 tid)
{
    for (u32 p = 0; p < kNumPriorities; ++p)
    {
        for (u32 t = 0; t < kNumThreads; ++t)
        {
            u32 u = (t + tid) & kThreadMask;
            if (!ms_pipes[u][p].IsEmpty())
            {
                return true;
            }
        }
    }
    return false;
}

static void WakeThreadsForTaskCompletion()
{
    i32 oldState = Load(ms_iWaitingForCompletes, MO_Relaxed);
    while (oldState > 0)
    {
        i32 newState = Max(oldState - 1, 0);
        if (CmpExStrong(ms_iWaitingForCompletes, oldState, newState, MO_Release, MO_Relaxed))
        {
            ms_completeTaskSema.Signal(1);
        }
        ThreadFenceAcquire();
    }
}

static void WakeThreadsForNewTasks()
{
    i32 oldState = Load(ms_iWaitingForNew, MO_Relaxed);
    while (oldState > 0)
    {
        i32 newState = Max(oldState - 1, 0);
        if (CmpExStrong(ms_iWaitingForNew, oldState, newState, MO_Release, MO_Relaxed))
        {
            ms_newTaskSema.Signal(1);
        }
        ThreadFenceAcquire();
    }

    WakeThreadsForTaskCompletion();
}

static void WaitForNewTasks(u32 tid)
{
    Inc(ms_iWaitingForNew, MO_Acquire);
    const u32 prevState = PushThreadState(tid, ThreadState_AwaitWork);

    if (HasTasks(tid))
    {
        Dec(ms_iWaitingForNew, MO_Release);
    }
    else
    {
        ms_newTaskSema.Wait();
    }

    PopThreadState(tid, prevState);
}

static void WaitForTaskCompletion(ITask* pTask, u32 tid)
{
    Inc(ms_iWaitingForCompletes, MO_Acquire);
    IncWait(pTask);
    const u32 prevState = PushThreadState(tid, ThreadState_AwaitDependency);

    if (IsComplete(pTask) || HasTasks(tid))
    {
        Dec(ms_iWaitingForCompletes, MO_Release);
    }
    else
    {
        ThreadFenceAcquire();
        ms_completeTaskSema.Wait();
        if (!IsComplete(pTask))
        {
            WakeThreadsForTaskCompletion();
        }
    }

    PopThreadState(tid, prevState);
    DecWait(pTask);
}

static void SplitAndAdd(
    u32 tid,
    TaskPriority priority,
    Subtask subtask,
    u32 rangeToSplit,
    u32 rangeToRun)
{
    i32 iAdd = 0;
    i32 iExec = 0;

    ITask* pTask = subtask.pTask;
    u32 begin = subtask.begin;
    const u32 end = subtask.end;
    const u32 totalRange = end - begin;
    rangeToSplit = Min(rangeToSplit, totalRange);
    rangeToRun = Min(rangeToRun, totalRange);

    IncExec(pTask);

    while (begin < end)
    {
        const u32 subRange = end - begin;
        const u32 splitRange = Min(rangeToSplit, subRange);
        const u32 runRange = Min(rangeToRun, subRange);

        Subtask splitChild = { pTask, begin, begin + splitRange };

        ++iAdd;
        IncExec(subtask.pTask);
        if (!ms_pipes[tid][priority].WriterTryWriteFront(splitChild))
        {
            if (iAdd > 1)
            {
                iAdd = 0;
                WakeThreadsForNewTasks();
            }

            ++iExec;
            pTask->Execute(begin, begin + runRange, tid);
            begin += runRange;
        }
        else
        {
            begin += splitRange;
        }
    }

    FetchSub(subtask.pTask->m_iExec, iExec + 1, MO_Release);

    WakeThreadsForNewTasks();
}

static bool TryRunTask(u32 tid, TaskPriority priority)
{
    bool ran = false;
    bool read = false;
    Subtask subtask = {};

    read = ms_pipes[tid][priority].WriterTryReadFront(subtask);

    if (!read)
    {
        for (u32 i = 0; i < kNumThreads; ++i)
        {
            u32 iPipe = (i + tid) & kThreadMask;
            read = ms_pipes[iPipe][priority].ReaderTryReadBack(subtask);
            if (read)
            {
                break;
            }
        }
    }

    if (read)
    {
        ITask* pTask = subtask.pTask;
        const u32 begin = subtask.begin;
        u32 end = subtask.end;

        ASSERT(pTask);
        ASSERT(begin < end);

        const u32 granularity = pTask->m_granularity;
        const u32 setSize = pTask->m_taskSize;
        const u32 runRange = RangeToRun(granularity, setSize);
        const u32 splitRange = RangeToSplit(granularity, setSize);
        const u32 totalRange = end - begin;

        if (runRange < totalRange)
        {
            const u32 remRange = totalRange - runRange;
            end = begin + runRange;
            Subtask next =
            {
                subtask.pTask,
                end,
                end + remRange,
            };
            SplitAndAdd(tid, priority, next, splitRange, runRange);
        }

        pTask->Execute(begin, end, tid);

        i32 prevExec = DecExec(pTask);
        ASSERT(prevExec > 0);

        if (prevExec == 1)
        {
            if (HasWaits(pTask))
            {
                WakeThreadsForTaskCompletion();
            }
        }

        ran = true;
    }

    return ran;
}

static bool TryRunTask(u32 tid)
{
    for (u32 p = 0; p < kNumPriorities; ++p)
    {
        if (TryRunTask(tid, (TaskPriority)p))
        {
            return true;
        }
    }
    return false;
}

static void AddTask(ITask* pTask)
{
    ASSERT(pTask);
    ASSERT(IsComplete(pTask));

    const u32 tid = ms_tid;

    const u32 prevState = PushThreadState(tid, ThreadState_Running);
    Store(pTask->m_iExec, 0, MO_Relaxed);

    const u32 granularity = pTask->m_granularity;
    const u32 setSize = pTask->m_taskSize;
    const u32 runRange = RangeToRun(granularity, setSize);
    const u32 splitRange = RangeToSplit(granularity, setSize);

    Subtask subtask = { pTask, 0, setSize };

    SplitAndAdd(tid, pTask->m_priority, subtask, splitRange, runRange);

    PopThreadState(tid, prevState);
}

static void WaitForTask(ITask* pTask, TaskPriority lowestToRun)
{
    ASSERT(pTask);

    const u32 tid = ms_tid;

    const u32 prevState = PushThreadState(tid, ThreadState_Running);
    ThreadFenceAcquire();

    if (!pTask->IsComplete())
    {
        lowestToRun = Max(lowestToRun, pTask->m_priority);

        u64 spins = 0;
        while (!pTask->IsComplete())
        {
            ++spins;
            for (u32 p = 0; p <= lowestToRun; ++p)
            {
                if (TryRunTask(tid, (TaskPriority)p))
                {
                    spins = 0;
                    break;
                }
            }
            if (spins > kMaxSpins)
            {
                spins = 0;
                WaitForTaskCompletion(pTask, tid);
            }
            else
            {
                OS::SpinWait(spins * kTicksPerSpin);
            }
        }
    }
    else
    {
        for (u32 p = 0; p <= lowestToRun; ++p)
        {
            if (TryRunTask(tid, (TaskPriority)p))
            {
                break;
            }
        }
    }

    PopThreadState(tid, prevState);
}

static void WaitForAll()
{
    const u32 tid = ms_tid;
    bool hasTasks = true;
    i32 nRunning = 0;
    u64 spins = 0;
    while (hasTasks || (nRunning > 0))
    {
        hasTasks = TryRunTask(tid);
        if (hasTasks)
        {
            spins = 0;
        }
        else
        {
            spins = Min(++spins, kMaxSpins);
            OS::SpinWait(kTicksPerSpin * spins);
        }

        nRunning = 0;
        for (u32 t = 1; t < kNumThreads; ++t)
        {
            if (t != tid)
            {
                u32 state = Load(ms_states[t].state, MO_Acquire);
                switch (state)
                {
                default:
                    break;
                case ThreadState_Running:
                case ThreadState_AwaitDependency:
                    ++nRunning;
                    break;
                }
            }
        }
    }
}

static void ThreadFn(void* pVoid)
{
    const u32 tid = (u32)(usize)(pVoid);
    ASSERT(tid > 0 && tid < kNumThreads);

    ms_tid = tid;

    Random::Seed();

    u64 spins = 0;
    while (Load(ms_running, MO_Relaxed))
    {
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else
        {
            ++spins;
            if (spins > kMaxSpins)
            {
                spins = 0;
                WaitForNewTasks(tid);
            }
            else
            {
                OS::SpinWait(spins * kTicksPerSpin);
            }
        }
    }

    Dec(ms_numThreadsRunning, MO_Release);
    Store(ms_states[tid].state, ThreadState_Stopped, MO_Release);
}

namespace TaskSystem
{
    void Submit(ITask* pTask)
    {
        AddTask(pTask);
    }

    void Await(ITask* pTask, TaskPriority lowestToRun)
    {
        WaitForTask(pTask, lowestToRun);
    }

    void AwaitAll()
    {
        WaitForAll();
    }

    static void Init()
    {
        ms_tid = 0u;
        ms_completeTaskSema.Open(0);
        ms_newTaskSema.Open(0);
        Store(ms_iWaitingForCompletes, 0, MO_Release);
        Store(ms_iWaitingForNew, 0, MO_Release);
        Store(ms_running, 1u, MO_Release);
        for (u32 t = 0u; t < kNumThreads; ++t)
        {
            for (u32 p = 0; p < kNumPriorities; ++p)
            {
                ms_pipes[t][p].Init();
            }
        }
        for (u32 t = 1u; t < kNumThreads; ++t)
        {
            usize tid = (usize)t;
            Store(ms_states[t].state, ThreadState_Running, MO_Release);
            ms_threads[t].Open(ThreadFn, (void*)tid);
            ++ms_numThreadsRunning;
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Store(ms_running, 0, MO_Release);
        while (Load(ms_numThreadsRunning, MO_Relaxed) > 0)
        {
            WakeThreadsForNewTasks();
        }

        for (u32 i = 1u; i < kNumThreads; ++i)
        {
            ms_threads[i].Join();
        }

        ms_newTaskSema.Close();
        ms_completeTaskSema.Close();
    }

    static constexpr System ms_system =
    {
        ToGuid("TaskSystem"),
        { 0, 0 },
        Init,
        Update,
        Shutdown,
    };
    static RegisterSystem ms_register(ms_system);
};
