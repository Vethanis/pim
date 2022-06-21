#include "threading/task.h"

#include "threading/thread.h"
#include "threading/event.h"
#include "threading/intrin.h"
#include "threading/sleep.h"
#include "common/atomics.h"
#include "containers/ptrqueue.h"
#include "allocator/allocator.h"
#include "math/scalar.h"
#include "common/profiler.h"
#include "common/cvars.h"

#include <string.h>

// fp flush and zeros mode
#include <xmmintrin.h>
#include <pmmintrin.h>

// ----------------------------------------------------------------------------

static i32 ms_numthreads;
static i32 ms_worksplit;
static i32 ms_numThreadsRunning;
static i32 ms_running;
static Event ms_waitPush;
static Event ms_waitDone;
static Thread ms_threads[kMaxThreads];
static PtrQueue ms_queues[kMaxThreads];

static pim_thread_local i32 ms_tid;

// ----------------------------------------------------------------------------

static bool ExecuteTask(Task* task)
{
    if (task)
    {
        const i32 wsize = task->worksize;
        const i32 gran = i1_max(1, wsize / ms_worksplit);
        const TaskExecuteFn fn = task->execute;

        i32 a = fetch_add_i32(&task->head, gran, MO_AcqRel);
        i32 b = i1_min(a + gran, wsize);
        while (a < b)
        {
            fn(task, a, b);

            const i32 count = b - a;
            const i32 prev = fetch_add_i32(&task->tail, count, MO_AcqRel);
            ASSERT(prev < wsize);
            if ((prev + count) >= wsize)
            {
                store_i32(&task->status, TaskStatus_Complete, MO_Release);
                Event_WakeAll(&ms_waitDone);
                break;
            }
            a = fetch_add_i32(&task->head, gran, MO_AcqRel);
            b = i1_min(a + gran, wsize);
        }
    }
    return task != NULL;
}

static bool TryRunTask(i32 tid)
{
    Task* task = PtrQueue_TryPop(&ms_queues[tid]);
    return ExecuteTask(task);
}

static i32 TaskLoop(void* arg)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    const i32 tid = inc_i32(&ms_numThreadsRunning, MO_AcqRel) + 1;
    ASSERT(tid);
    ms_tid = tid;

    while (load_i32(&ms_running, MO_Acquire))
    {
        if (!TryRunTask(tid))
        {
            Event_Wait(&ms_waitPush);
        }
    }

    dec_i32(&ms_numThreadsRunning, MO_AcqRel);

    return 0;
}

// ----------------------------------------------------------------------------

i32 Task_ThreadId(void)
{
    return ms_tid;
}

i32 Task_ThreadCount(void)
{
    ASSERT(ms_numthreads > 0);
    return ms_numthreads;
}

TaskStatus Task_Stat(const void* pbase)
{
    ASSERT(pbase);
    Task const *const task = pbase;
    return (TaskStatus)load_i32(&task->status, MO_Acquire);
}

void Task_Submit(void* pbase, TaskExecuteFn execute, i32 worksize)
{
    ASSERT(execute);
    Task *const task = pbase;
    if (task && worksize > 0)
    {
        ASSERT(Task_Stat(task) == TaskStatus_Init);
        store_i32(&task->status, TaskStatus_Exec, MO_Release);
        task->execute = execute;
        store_i32(&task->worksize, worksize, MO_Release);
        store_i32(&task->head, 0, MO_Release);
        store_i32(&task->tail, 0, MO_Release);

        bool anyFull = false;
        bool resubmit[kMaxThreads] = { 0 };

        const i32 tid = ms_tid;
        const i32 numthreads = ms_numthreads;
        for (i32 t = 0; t < numthreads; ++t)
        {
            bool full = !PtrQueue_TryPush(&ms_queues[t], task);
            anyFull |= full;
            resubmit[t] = full;
        }

        while (anyFull)
        {
            anyFull = false;
            TaskSys_Schedule();
            for (i32 t = 0; t < numthreads; ++t)
            {
                if (resubmit[t])
                {
                    TryRunTask(t);
                    bool full = !PtrQueue_TryPush(&ms_queues[t], task);
                    anyFull |= full;
                    resubmit[t] = full;
                }
            }
        }
    }
}

ProfileMark(pm_exec, Task_Exec);
ProfileMark(pm_await, Task_Wait);
void Task_Await(void* pbase)
{
    Task* task = pbase;
    if (task)
    {
        ProfileBegin(pm_exec);
        ExecuteTask(task);
        ProfileEnd(pm_exec);

        ProfileBegin(pm_await);
        const i32 tid = ms_tid;
        while (Task_Stat(task) != TaskStatus_Complete)
        {
            if (!TryRunTask(tid))
            {
                Event_Wait(&ms_waitDone);
            }
        }
        ProfileEnd(pm_await);
    }
}

void Task_Run(void* pbase, TaskExecuteFn fn, i32 worksize)
{
    Task* task = pbase;
    ASSERT(task);
    ASSERT(fn);
    ASSERT(worksize >= 0);
    if (worksize > 0)
    {
        Task_Submit(task, fn, worksize);
        TaskSys_Schedule();
        Task_Await(task);
    }
}

ProfileMark(pm_schedule, TaskSys_Schedule)
void TaskSys_Schedule(void)
{
    ProfileBegin(pm_schedule);

    Event_WakeAll(&ms_waitPush);

    ProfileEnd(pm_schedule);
}

void TaskSys_Init(void)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    Intrin_BeginClockRes(1);

    Event_New(&ms_waitPush);
    Event_New(&ms_waitDone);
    store_i32(&ms_running, 1, MO_Release);

    const i32 numthreads = Thread_HardwareCount();
    ms_numthreads = numthreads;
    ms_worksplit = numthreads * numthreads;

    const i32 kQueueSize = 64;
    PtrQueue_New(ms_queues + 0, EAlloc_Perm, kQueueSize);
    for (i32 t = 1; t < numthreads; ++t)
    {
        PtrQueue_New(ms_queues + t, EAlloc_Perm, kQueueSize);
        Thread_New(ms_threads + t, TaskLoop, NULL);
    }
}

void TaskSys_Update(void)
{

}

void TaskSys_Shutdown(void)
{
    store_i32(&ms_running, 0, MO_Release);
    Event_WakeAll(&ms_waitDone);
    Event_WakeAll(&ms_waitPush);
    const i32 numthreads = ms_numthreads;
    for (i32 t = 1; t < numthreads; ++t)
    {
        Thread_Join(&ms_threads[t]);
        PtrQueue_Del(&ms_queues[t]);
    }
    PtrQueue_Del(&ms_queues[0]);

    Event_Del(&ms_waitPush);
    Event_Del(&ms_waitDone);
    Intrin_EndClockRes(1);

    memset(ms_threads, 0, sizeof(ms_threads));
    memset(ms_queues, 0, sizeof(ms_queues));
    ms_numthreads = 0;
}

ProfileMark(pm_endframe, TaskSys_EndFrame)
void TaskSys_EndFrame(void)
{
    ProfileBegin(pm_endframe);

    // clear out backlog, in case a queue piles up
    const i32 numthreads = ms_numthreads;
    for (i32 tid = 0; tid < numthreads; ++tid)
    {
        while (TryRunTask(tid)) {}
    }

    ProfileEnd(pm_endframe);
}
