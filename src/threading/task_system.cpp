#include "threading/task_system.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/pipe.h"
#include "components/system.h"
#include "common/random.h"

// ----------------------------------------------------------------------------

enum ThreadState : u32
{
    ThreadState_Running,
    ThreadState_AwaitDependency,
    ThreadState_AwaitWork,
    ThreadState_Stopped
};

static constexpr u64 kMaxSpins = 10;
static constexpr u64 kTicksPerSpin = 100;
static constexpr u32 kNumThreads = 16;
static constexpr u32 kThreadMask = kNumThreads - 1u;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static PtrPipe<2048> ms_queue;

static OS::Event ms_waitPush;
static OS::Event ms_waitExec;
static OS::Event ms_waitPop;

static i32 ms_numThreadsRunning;
static u32 ms_running;

static thread_local u32 ms_tid;

// ----------------------------------------------------------------------------

static TaskState GetState(const ITask* pTask)
{
    ASSERT(pTask);
    return (TaskState)Load(pTask->m_state, MO_Acquire);
}

static bool IsComplete(const ITask* pTask)
{
    return TaskState_Complete == GetState(pTask);
}

static bool IsInProgress(const ITask* pTask)
{
    switch (GetState(pTask))
    {
    case TaskState_Submit:
    case TaskState_Execute:
        return true;
    }
    return false;
}

static bool IsInitOrComplete(ITask* pTask)
{
    switch (GetState(pTask))
    {
    case TaskState_Init:
    case TaskState_Complete:
        return true;
    }
    return false;
}

TaskState ITask::GetState() const { return ::GetState(this); }
bool ITask::IsComplete() const { return ::IsComplete(this); }
bool ITask::IsInProgress() const { return ::IsInProgress(this); }

// ----------------------------------------------------------------------------

static bool TryRunTask(u32 tid)
{
    ITask* pTask = (ITask*)ms_queue.TryPop();
    if (pTask)
    {
        ms_waitPop.WakeOne();

        ASSERT(pTask);
        Store(pTask->m_state, TaskState_Execute, MO_Release);
        pTask->Execute(tid);
        Store(pTask->m_state, TaskState_Complete, MO_Release);

        if (Load(pTask->m_waits, MO_Relaxed) > 0)
        {
            ms_waitExec.WakeAll();
        }

        return true;
    }
    return false;
}

static void AddTask(ITask* pTask)
{
    ASSERT(pTask);
    ASSERT(IsInitOrComplete(pTask));

    Store(pTask->m_state, TaskState_Submit, MO_Release);

    const u32 tid = ms_tid;

    u64 spins = 0;
    while (!ms_queue.TryPush(pTask))
    {
        ++spins;
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else if (spins > kMaxSpins)
        {
            ms_waitPop.Wait();
            spins = 0;
        }
        else
        {
            OS::Spin(spins * kTicksPerSpin);
        }
    }

    ms_waitPush.WakeOne();
}

static void WaitForTask(ITask* pTask)
{
    ASSERT(pTask);

    const u32 tid = ms_tid;

    u64 spins = 0;
    while (!IsComplete(pTask))
    {
        ++spins;
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else if (spins > kMaxSpins)
        {
            Inc(pTask->m_waits, MO_Acquire);
            ms_waitExec.Wait();
            Dec(pTask->m_waits, MO_Release);
            spins = 0;
        }
        else
        {
            OS::Spin(spins * kTicksPerSpin);
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
                ms_waitPush.Wait();
                spins = 0;
            }
            else
            {
                OS::Spin(spins * kTicksPerSpin);
            }
        }
    }

    Dec(ms_numThreadsRunning, MO_Release);
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

    static void Init()
    {
        ms_tid = 0u;
        ms_waitPush.Open();
        ms_waitExec.Open();
        ms_waitPop.Open();
        Store(ms_running, 1u, MO_Release);
        ms_queue.Init();
        for (u32 t = 0u; t < kNumThreads; ++t)
        {
            usize tid = (usize)t;
            ms_threads[t].Open(ThreadFn, (void*)tid);
            ++ms_numThreadsRunning;
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Store(ms_running, 0u, MO_Release);
        while (Load(ms_numThreadsRunning, MO_Relaxed) > 0)
        {
            ms_waitPush.WakeAll();
            ms_waitExec.WakeAll();
            ms_waitPop.WakeAll();
        }

        for (u32 i = 0u; i < kNumThreads; ++i)
        {
            ms_threads[i].Join();
        }

        ms_queue.Clear();
        ms_waitPush.Close();
        ms_waitExec.Close();
        ms_waitPop.Close();
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
