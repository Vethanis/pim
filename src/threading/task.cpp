#include "threading/task.h"
#include "os/thread.h"
#include "os/atomics.h"
#include "containers/pipe.h"
#include "components/system.h"
#include "common/random.h"

struct ITaskFriend
{
    static i32& GetWaits(ITask* pTask)
    {
        return pTask->m_waits;
    }
    static i32& GetExec(ITask* pTask)
    {
        return pTask->m_exec;
    }
    static i32 GetBegin(ITask* pTask)
    {
        return pTask->m_begin;
    }
    static i32 GetEnd(ITask* pTask)
    {
        return pTask->m_end;
    }
};

static i32& GetWaits(ITask* pTask) { return ITaskFriend::GetWaits(pTask); }
static i32& GetExec(ITask* pTask) { return ITaskFriend::GetExec(pTask); }
static i32 GetBegin(ITask* pTask) { return ITaskFriend::GetBegin(pTask); }
static i32 GetEnd(ITask* pTask) { return ITaskFriend::GetEnd(pTask); }

// ----------------------------------------------------------------------------

bool ITask::IsComplete() const { return Load(m_exec) == 0; }
bool ITask::IsInProgress() const { return Load(m_exec) > 0; }
bool ITask::IsInitOrComplete() const
{
    return Load(m_exec) == 0;
}

void ITask::SetRange(i32 begin, i32 end)
{
    ASSERT(IsInitOrComplete());
    m_begin = begin;
    m_end = end;
}

// ----------------------------------------------------------------------------

struct Subtask
{
    ITask* pTask;
    i32 begin;
    i32 end;
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
static constexpr u32 kNumThreads = 32;
static constexpr u32 kThreadMask = kNumThreads - 1u;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static Pipe<Subtask, 8> ms_queues[kNumThreads];

static OS::Event ms_waitPush;
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

    for (u32 t = 0u; t < kNumThreads; ++t)
    {
        const u32 j = (t + tid + hint) & kThreadMask;

        Subtask subtask = {};
        if (ms_queues[t].TryPop(subtask))
        {
            ms_hint = t;

            ITask* pTask = subtask.pTask;
            pTask->Execute(subtask.begin, subtask.end);

            if (Dec(GetExec(pTask)) == 1)
            {
                ms_waitExec.WakeAll();
            }

            return true;
        }
    }

    return false;
}

static void Insert(Subtask subtask)
{
    const u32 tid = ms_tid;
    u64 spins = 0;
    const u32 hint = ms_hint;
    while (true)
    {
        for (u32 t = 0; t < kNumThreads; ++t)
        {
            const u32 j = (t + tid + hint) & kThreadMask;
            if (ms_queues[j].TryPush(subtask))
            {
                ms_hint = t;
                return;
            }
        }
        ms_waitPush.WakeAll();
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

static void AddTask(ITask* pTask)
{
    ASSERT(pTask);
    ASSERT(pTask->IsInitOrComplete());

    i32 begin = GetBegin(pTask);
    const i32 end = GetEnd(pTask);
    const i32 count = end - begin;
    ASSERT(count >= 0);

    i32& iExec = GetExec(pTask);
    Store(iExec, 1);

    if (count == 0)
    {
        Subtask subtask = {};
        subtask.pTask = pTask;
        subtask.begin = begin;
        subtask.end = end;
        Inc(iExec);
        Insert(subtask);
    }
    else
    {
        const i32 delta = Max(1, count / (i32)kNumThreads);
        while (begin < end)
        {
            const i32 next = Min(begin + delta, end);
            Subtask subtask = {};
            subtask.pTask = pTask;
            subtask.begin = begin;
            subtask.end = next;
            begin = next;
            Inc(iExec);
            Insert(subtask);
        }
    }

    Dec(iExec, MO_Release);

    ms_waitPush.WakeAll();
}

static void WaitForTask(ITask* pTask)
{
    ASSERT(pTask);

    i32& waits = ITaskFriend::GetWaits(pTask);
    Inc(waits, MO_Acquire);

    while (!pTask->IsComplete())
    {
        ms_waitExec.Wait();
    }

    Dec(waits, MO_Release);
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
            ms_waitPush.Wait();
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
            ms_waitPush.Open();
            ms_waitExec.Open();
            Store(ms_running, 1u);
            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_queues[t].Init();
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
                ms_waitPush.WakeAll();
                ms_waitExec.WakeAll();
                OS::SwitchThread();
            }

            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_threads[t].Join();
            }

            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                ms_queues[t].Clear();
            }

            ms_waitPush.Close();
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
