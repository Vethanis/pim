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

void ITask::SetRange(i32 begin, i32 end)
{
    ASSERT(!IsInProgress());
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
static constexpr u32 kThreadMask = kNumThreads - 1u;
static constexpr u32 kTaskSplit = kNumThreads * (kNumThreads - 1u);

using pipe_t = Pipe<Subtask, kNumThreads>;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static pipe_t ms_queues[kNumThreads];

static OS::Event ms_waitPush;

static i32 ms_numThreadsRunning;
static i32 ms_numThreadsSleeping;
static u32 ms_running;

static thread_local u32 ms_tid;

// ----------------------------------------------------------------------------

u32 ThreadId()
{
    return ms_tid;
}

u32 NumActiveThreads()
{
    return Load(ms_numThreadsRunning, MO_Relaxed) - Load(ms_numThreadsSleeping, MO_Relaxed);
}

static bool TryRunTask(u32 tid)
{
    pipe_t* const queues = ms_queues;

    for (u32 t = 0u; t < kNumThreads; ++t)
    {
        const u32 j = (t + tid) & kThreadMask;

        Subtask subtask = {};
        if (queues[j].TryPop(subtask))
        {
            ITask* pTask = subtask.pTask;
            pTask->Execute(subtask.begin, subtask.end);
            Dec(GetExec(pTask));

            return true;
        }
    }

    return false;
}

static void Insert(Subtask subtask, u32 tid)
{
    pipe_t* const queues = ms_queues;

    u64 spins = 0;
    while (true)
    {
        for (u32 t = 0; t < kNumThreads; ++t)
        {
            const u32 j = (t + tid) & kThreadMask;
            if (queues[j].TryPush(subtask))
            {
                return;
            }
        }
        ms_waitPush.WakeOne();
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
    }
}

static void AddTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);

    i32& iExec = GetExec(pTask);
    ASSERT(Load(iExec) == 0);

    i32 begin = GetBegin(pTask);
    const i32 end = GetEnd(pTask);
    const i32 count = end - begin;
    ASSERT(count >= 0);

    if (count == 0)
    {
        Subtask subtask = {};
        subtask.pTask = pTask;
        subtask.begin = begin;
        subtask.end = end;
        Inc(iExec);
        Insert(subtask, tid);
    }
    else
    {
        const i32 delta = Max(1, count / (i32)kTaskSplit);
        while (begin < end)
        {
            const i32 next = Min(begin + delta, end);
            Subtask subtask = {};
            subtask.pTask = pTask;
            subtask.begin = begin;
            subtask.end = next;
            begin = next;
            Inc(iExec);
            Insert(subtask, tid);
        }
    }

    ms_waitPush.WakeOne();
}

static void WaitForTask(ITask* pTask, u32 tid)
{
    ASSERT(pTask);

    i32& waits = GetWaits(pTask);
    Inc(waits);

    if (Load(GetExec(pTask)) > 0)
    {
        ms_waitPush.Wake(2);
    }

    u64 spins = 0;
    while (Load(GetExec(pTask)) > 0)
    {
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
    }

    Dec(waits);
}

static void TaskLoop(void* pVoid)
{
    Inc(ms_numThreadsRunning);

    Random::Seed();

    ms_tid = (u32)((isize)pVoid);
    const u32 tid = ms_tid;
    ASSERT(tid != 0u);

    u64 spins = 0;
    while (Load(ms_running))
    {
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else if (spins < kMaxSpins)
        {
            OS::Spin(++spins * kTicksPerSpin);
        }
        else
        {
            Inc(ms_numThreadsSleeping);
            ms_waitPush.Wait();
            Dec(ms_numThreadsSleeping);
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
            Store(ms_running, 1u);

            pipe_t* const queues = ms_queues;
            OS::Thread* const threads = ms_threads;
            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                queues[t].Init();
            }
            for (u32 t = 1u; t < kNumThreads; ++t)
            {
                threads[t].Open(TaskLoop, (void*)((isize)t));
            }
        }

        void Update() final {}

        void Shutdown() final
        {
            Store(ms_running, 0u);
            while (Load(ms_numThreadsRunning, MO_Relaxed) > 0)
            {
                ms_waitPush.WakeAll();
                OS::SwitchThread();
            }

            pipe_t* const queues = ms_queues;
            OS::Thread* const threads = ms_threads;
            for (u32 t = 1u; t < kNumThreads; ++t)
            {
                threads[t].Join();
            }
            for (u32 t = 0u; t < kNumThreads; ++t)
            {
                queues[t].Clear();
            }

            ms_waitPush.Close();
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
