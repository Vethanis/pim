#include "threading/task_system.h"
#include "os/thread.h"
#include "containers/pipe.h"
#include "components/system.h"

static constexpr u32 kNumThreads = 32;
static constexpr u32 kThreadMask = kNumThreads - 1u;

static OS::Thread ms_threads[kNumThreads];
static ScatterPipe<Task, 64> ms_pipes[kNumThreads];
static OS::LightSema ms_signal;
static u32 ms_running;
static u32 ms_robin;

static void ThreadFn(void* pVoid)
{
    const u32 tid = (u32)(usize)(pVoid);
    Task task = {};
    while (true)
    {
        ms_signal.Wait();
        if (!Load(ms_running, MO_Relaxed))
        {
            break;
        }

        u64 spins = 0;
        while (spins < 10)
        {
            for (u32 i = 0; i < kNumThreads; ++i)
            {
                u32 j = (i + tid) & kThreadMask;
                while (ms_pipes[j].ReaderTryReadBack(task))
                {
                    task.Run(task);
                    spins = 0;
                }
            }

            if (spins > 0)
            {
                OS::SpinWait(spins * 100);
            }
            ++spins;
        }
    }
}

static bool TrySchedule(const Task& task)
{
    u32 i = ms_robin++;
    u64 spins = 0;
    while (spins < 10)
    {
        for (u32 j = 0; j < kNumThreads; ++j)
        {
            u32 k = (i + j) & kThreadMask;
            if (ms_pipes[k].WriterTryWriteFront(task))
            {
                ms_signal.Signal();
                return true;
            }
        }
        ++spins;
        OS::SpinWait(spins * 100);
    }
    return false;
}

namespace TaskSystem
{
    void Schedule(const Task& task)
    {
        ASSERT(task.Run);
        bool scheduled = TrySchedule(task);
        ASSERT(scheduled);
    }

    static void Init()
    {
        ms_robin = 0;
        Store(ms_running, 1u, MO_Release);
        ms_signal.Open();
        for (u32 i = 0; i < kNumThreads; ++i)
        {
            ms_pipes[i].Init();
        }
        for (u32 i = 0; i < kNumThreads; ++i)
        {
            usize tid = (usize)i;
            ms_threads[i].Open(ThreadFn, (void*)tid);
        }
    }

    static void Update()
    {

    }

    static void Shutdown()
    {
        Store(ms_running, 0u, MO_Release);
        ms_signal.Signal(kNumThreads);
        for (u32 i = 0; i < kNumThreads; ++i)
        {
            ms_threads[i].Join();
        }
        ms_signal.Close();
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
