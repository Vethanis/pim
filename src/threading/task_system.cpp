#include "threading/task_system.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/queue.h"
#include "components/system.h"
#include "common/random.h"

// ----------------------------------------------------------------------------

struct TaskQueue
{
    OS::Mutex m_lock;
    Queue<ITask*> m_queue;

    void Init()
    {
        m_lock.Open();
        m_queue.Init(Alloc_Pool);
    }

    void Shutdown()
    {
        m_lock.Close();
        m_queue.Reset();
    }

    bool TryPop(ITask*& dst)
    {
        bool popped = false;
        if (m_lock.TryLock())
        {
            if (m_queue.HasItems())
            {
                dst = m_queue.Pop();
                ASSERT(dst);
                popped = true;
            }
            m_lock.Unlock();
        }
        return popped;
    }

    bool TryPush(ITask* src)
    {
        ASSERT(src);
        bool pushed = false;
        if (m_lock.TryLock())
        {
            m_queue.Push(src);
            pushed = true;
            m_lock.Unlock();
        }
        return pushed;
    }
};

// ----------------------------------------------------------------------------

enum ThreadState : u32
{
    ThreadState_Running,
    ThreadState_AwaitDependency,
    ThreadState_AwaitWork,
    ThreadState_Stopped
};

struct alignas(64) thread_state_t
{
    u32 state;
    u8 padding[64 - 4];
};

static constexpr u64 kMaxSpins = 10;
static constexpr u64 kTicksPerSpin = 100;
static constexpr u32 kNumThreads = 16;
static constexpr u32 kThreadMask = kNumThreads - 1u;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static thread_state_t ms_states[kNumThreads];
static TaskQueue ms_queues[kNumThreads];

static OS::MultiEvent ms_waitNew;
static OS::MultiEvent ms_waitComplete;

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

static bool IsComplete(const ITask* pTask)
{
    return TaskState_Complete == Load(pTask->m_state, MO_Acquire);
}

bool ITask::IsComplete() const { return ::IsComplete(this); }

TaskState ITask::GetState() const
{
    return (TaskState)Load(m_state, MO_Acquire);
}

void ITask::AwaitIfQueued()
{
    switch (GetState())
    {
    case TaskState_Submit:
    case TaskState_Execute:
        TaskSystem::Await(this);
        break;
    }
}

void ITask::SubmitIfNotQueued()
{
    switch (GetState())
    {
    case TaskState_Init:
    case TaskState_Complete:
        TaskSystem::Submit(this);
        break;
    }
}

static bool HasWaits(const ITask* pTask)
{
    const i32 iWait = Load(pTask->m_iWait, MO_Acquire);
    ASSERT(iWait >= 0);
    return iWait > 0;
}

static i32 IncWait(ITask* pTask)
{
    const i32 prev = Inc(pTask->m_iWait, MO_Acquire);
    ASSERT(prev >= 0);
    return prev;
}

static i32 DecWait(ITask* pTask)
{
    const i32 prev = Dec(pTask->m_iWait, MO_Release);
    ASSERT(prev > 0);
    return prev;
}

static bool TryPushTask(ITask* pTask, u32 tid)
{
    for (u32 i = 0; i < kNumThreads; ++i)
    {
        u32 u = (tid + i) & kThreadMask;
        if (ms_queues[u].TryPush(pTask))
        {
            Store(pTask->m_state, TaskState_Submit, MO_Relaxed);
            return true;
        }
    }
    return false;
}

static ITask* TryPopTask(u32 tid)
{
    for (u32 i = 0; i < kNumThreads; ++i)
    {
        u32 u = (tid + i) & kThreadMask;
        ITask* pTask = nullptr;
        if (ms_queues[u].TryPop(pTask))
        {
            Store(pTask->m_state, TaskState_Execute, MO_Relaxed);
            return pTask;
        }
    }
    return nullptr;
}

static void WakeForComplete()
{
    ms_waitComplete.Signal();
}

static void WakeForNew()
{
    ms_waitNew.Signal();
    WakeForComplete();
}

static void WaitForNew(u32 tid)
{
    const u32 prevState = PushThreadState(tid, ThreadState_AwaitWork);
    ms_waitNew.Wait();
    PopThreadState(tid, prevState);
}

static void WaitForComplete(ITask* pTask, u32 tid)
{
    IncWait(pTask);
    const u32 prevState = PushThreadState(tid, ThreadState_AwaitDependency);
    ms_waitComplete.Wait();
    PopThreadState(tid, prevState);
    DecWait(pTask);
}

static bool TryRunTask(u32 tid)
{
    ITask* pTask = TryPopTask(tid);
    if (pTask)
    {
        pTask->Execute(tid);

        Store(pTask->m_state, TaskState_Complete, MO_Release);

        if (HasWaits(pTask))
        {
            WakeForComplete();
        }

        return true;
    }

    return false;
}

static bool IsInitOrComplete(ITask* pTask)
{
    switch (Load(pTask->m_state, MO_Relaxed))
    {
    case TaskState_Init:
    case TaskState_Complete:
        return true;
    }
    return false;
}

static void AddTask(ITask* pTask)
{
    ASSERT(pTask);
    ASSERT(IsInitOrComplete(pTask));
    Store(pTask->m_state, TaskState_Init, MO_Relaxed);

    const u32 tid = ms_tid;

    const u32 prevState = PushThreadState(tid, ThreadState_Running);

    u64 spins = 0;
    while (!TryPushTask(pTask, tid))
    {
        ++spins;
        OS::SpinWait(spins * kTicksPerSpin);
    }

    WakeForNew();

    PopThreadState(tid, prevState);
}

static void WaitForTask(ITask* pTask)
{
    ASSERT(pTask);

    const u32 tid = ms_tid;

    const u32 prevState = PushThreadState(tid, ThreadState_Running);
    ThreadFenceAcquire();

    u64 spins = 0;
    while (!pTask->IsComplete())
    {
        ++spins;
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else if (spins > kMaxSpins)
        {
            spins = 0;
            WaitForComplete(pTask, tid);
        }
        else
        {
            OS::SpinWait(spins * kTicksPerSpin);
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
        for (u32 t = 0; t < kNumThreads; ++t)
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
    ASSERT(tid >= 0 && tid < kNumThreads);

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
                WaitForNew(tid);
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

    void Await(ITask* pTask)
    {
        WaitForTask(pTask);
    }

    void AwaitAll()
    {
        WaitForAll();
    }

    static void Init()
    {
        ms_tid = 0u;
        ms_waitComplete.Open();
        ms_waitNew.Open();
        Store(ms_running, 1u, MO_Release);
        for (u32 t = 0u; t < kNumThreads; ++t)
        {
            ms_queues[t].Init();
        }
        for (u32 t = 0u; t < kNumThreads; ++t)
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
            WakeForNew();
        }

        for (u32 i = 0u; i < kNumThreads; ++i)
        {
            ms_threads[i].Join();
        }
        for (u32 i = 0u; i < kNumThreads; ++i)
        {
            ms_queues[i].Shutdown();
        }

        ms_waitNew.Close();
        ms_waitComplete.Close();
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
