#include "threading/task.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/pipe.h"
#include "components/system.h"
#include "common/random.h"

TaskState ITask::GetState() const { return (TaskState)Load(m_state, MO_Relaxed); }

bool ITask::IsComplete() const { return GetState() == TaskState_Complete; }

bool ITask::IsInProgress() const
{
    switch (GetState())
    {
    case TaskState_Submit:
    case TaskState_Execute:
        return true;
    }
    return false;
}

bool ITask::IsInitOrComplete() const
{
    switch (GetState())
    {
    case TaskState_Init:
    case TaskState_Complete:
        return true;
    }
    return false;
}

TaskPriority ITask::GetPriority() const
{
    return (TaskPriority)Load(m_priority, MO_Relaxed);
}

struct ITaskFriend
{
    static u32& GetState(ITask* pTask)
    {
        return pTask->m_state;
    }
    static i32& GetWaits(ITask* pTask)
    {
        return pTask->m_waits;
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

static constexpr u64 kMaxSpins = 10;
static constexpr u64 kTicksPerSpin = 100;
static constexpr u32 kNumThreads = 16;
static constexpr u32 kThreadMask = kNumThreads - 1u;
static constexpr u32 kNumPriority = TaskPriority_COUNT;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static PtrPipe<8> ms_queues[kNumPriority][kNumThreads];

static OS::Event ms_waitPush[kNumThreads];
static OS::Event ms_waitExec;

static i32 ms_numThreadsRunning;
static u32 ms_running;

static thread_local u32 ms_tid;
static thread_local u32 ms_hint;

// ----------------------------------------------------------------------------

static bool TryRunTask()
{
    const u32 tid = ms_tid;
    const u32 hint = ms_hint;

    for (u32 p = 0u; p < kNumPriority; ++p)
    {
        for (u32 t = 0u; t < kNumThreads; ++t)
        {
            const u32 j = (t + tid + hint) & kThreadMask;

            ITask* pTask = reinterpret_cast<ITask*>(ms_queues[p][t].TryPop());
            if (pTask)
            {
                u32& state = ITaskFriend::GetState(pTask);
                i32& waits = ITaskFriend::GetWaits(pTask);

                Store(state, TaskState_Execute);
                pTask->Execute();
                Store(state, TaskState_Complete);

                if (Load(waits) > 0)
                {
                    ms_waitExec.WakeAll();
                }

                ms_hint = t;

                return true;
            }
        }
    }
    return false;
}

static void AddTask(ITask* pTask)
{
    ASSERT(pTask);
    ASSERT(pTask->IsInitOrComplete());

    Store(ITaskFriend::GetState(pTask), TaskState_Submit);

    const u32 tid = ms_tid;
    const u32 p = pTask->GetPriority();
    u64 spins = 0;

    while (true)
    {
        const u32 hint = ms_hint;
        for (u32 t = 0; t < kNumThreads; ++t)
        {
            const u32 j = (t + tid + hint) & kThreadMask;
            if (ms_queues[p][j].TryPush(pTask))
            {
                ms_waitPush[t].WakeOne();
                ms_hint = t;
                return;
            }
        }
        if (TryRunTask())
        {
            spins = 0;
        }
        else
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
    }
}

static void WaitForTask(ITask* pTask)
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
            i32& waits = ITaskFriend::GetWaits(pTask);
            Inc(waits, MO_Acquire);
            ms_waitExec.Wait();
            Dec(waits, MO_Release);
            spins = 0;
        }
    }
}

static void ThreadFn(void* pVoid)
{
    Inc(ms_numThreadsRunning);

    Random::Seed();

    ms_tid = (u32)((isize)pVoid);
    const u32 tid = ms_tid;

    u64 spins = 0;
    while (Load(ms_running, MO_Relaxed))
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
            ms_waitPush[tid].Wait();
            spins = 0;
        }
    }

    Dec(ms_numThreadsRunning);
}

namespace TaskSystem
{
    struct System final : ISystem
    {
        System() : ISystem("TaskSystem") {}

        void Init() final
        {
            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_waitPush[t].Open();
            }
            ms_waitExec.Open();
            Store(ms_running, 1u);
            for (u32 p = 0u; p < kNumPriority; ++p)
            {
                for (u32 t = 0u; t < kNumThreads; ++t)
                {
                    ms_queues[p][t].Init();
                }
            }
            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_threads[t].Open(ThreadFn, (void*)((isize)t));
            }
        }

        void Update() final {}

        void Shutdown() final
        {
            Store(ms_running, 0u);
            while (Load(ms_numThreadsRunning) > 0)
            {
                for (u32 t = 0; t < kNumThreads; ++t)
                {
                    ms_waitPush[t].WakeAll();
                }
                ms_waitExec.WakeAll();
                OS::SwitchThread();
            }

            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_threads[t].Join();
            }

            for (u32 p = 0u; p < kNumPriority; ++p)
            {
                for (u32 t = 0u; t < kNumThreads; ++t)
                {
                    ms_queues[p][t].Clear();
                }
            }

            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_waitPush[t].Close();
            }

            ms_waitExec.Close();
        }
    };
    static System ms_system;

    void Submit(ITask* pTask)
    {
        AddTask(pTask);
    }
    void Await(ITask* pTask)
    {
        WaitForTask(pTask);
    }
};
