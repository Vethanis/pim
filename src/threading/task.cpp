#include "threading/task.h"
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

// ----------------------------------------------------------------------------

static bool TryRunTask()
{
    Task* pTask = (Task*)ms_queue.TryPop();
    if (pTask)
    {
        ms_waitPop.WakeOne();

        Store(pTask->m_state, TaskState_Execute);
        pTask->Execute();
        Store(pTask->m_state, TaskState_Complete);

        if (Load(pTask->m_waits) > 0)
        {
            ms_waitExec.WakeAll();
        }

        return true;
    }
    return false;
}

static void AddTask(Task* pTask)
{
    ASSERT(pTask);
    ASSERT(pTask->IsInitOrComplete());

    Store(pTask->m_state, TaskState_Submit);

    u64 spins = 0;
    while (!ms_queue.TryPush(pTask))
    {
        if (TryRunTask())
        {
            spins = 0;
        }
        else if (spins < kMaxSpins)
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
        else
        {
            ms_waitPop.Wait();
            spins = 0;
        }
    }

    ms_waitPush.WakeOne();
}

static void WaitForTask(Task* pTask)
{
    ASSERT(pTask);
    ASSERT(pTask->GetState() != TaskState_Init);

    u64 spins = 0;
    while (!pTask->IsComplete())
    {
        if (TryRunTask())
        {
            spins = 0;
        }
        else if (spins < kMaxSpins)
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
        else
        {
            Inc(pTask->m_waits);
            ms_waitExec.Wait();
            Dec(pTask->m_waits);
            spins = 0;
        }
    }
}

static void ThreadFn(void*)
{
    Random::Seed();

    u64 spins = 0;
    while (Load(ms_running))
    {
        if (TryRunTask())
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

    Dec(ms_numThreadsRunning);
}

namespace TaskSystem
{
    void Submit(Task* pTask)
    {
        AddTask(pTask);
    }

    void Await(Task* pTask)
    {
        WaitForTask(pTask);
    }

    static void Init()
    {
        ms_waitPush.Open();
        ms_waitExec.Open();
        ms_waitPop.Open();
        Store(ms_running, 1u);
        ms_queue.Init();
        for (u32 t = 0u; t < kNumThreads; ++t)
        {
            ms_threads[t].Open(ThreadFn, nullptr);
            ++ms_numThreadsRunning;
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Store(ms_running, 0u);
        while (Load(ms_numThreadsRunning) > 0)
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

    DEFINE_SYSTEM("TaskSystem", {}, Init, Update, Shutdown)
};
