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

#include <string.h>

// fp flush and zeros mode
#include <xmmintrin.h>
#include <pmmintrin.h>

// ----------------------------------------------------------------------------

static i32 ms_numthreads;
static i32 ms_worksplit;
static i32 ms_numThreadsRunning;
static i32 ms_running;
static event_t ms_waitPush;
static event_t ms_waitDone;
static thread_t ms_threads[kMaxThreads];
static ptrqueue_t ms_queues[kMaxThreads];

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
                event_wakeall(&ms_waitDone);
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
    Task* task = ptrqueue_trypop(&ms_queues[tid]);
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
            event_wait(&ms_waitPush);
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

        const i32 tid = ms_tid;
        const i32 numthreads = ms_numthreads;
        for (i32 t = 0; t < numthreads; ++t)
        {
            if (t != tid)
            {
                if (!ptrqueue_trypush(&ms_queues[t], task))
                {
                    INTERRUPT();
                }
            }
        }
    }
}

ProfileMark(pm_exec, task_exec);
ProfileMark(pm_await, Task_Await);
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
                event_wait(&ms_waitDone);
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

    event_wakeall(&ms_waitPush);

    ProfileEnd(pm_schedule);
}

void TaskSys_Init(void)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    intrin_clockres_begin(1);

    event_create(&ms_waitPush);
    event_create(&ms_waitDone);
    store_i32(&ms_running, 1, MO_Release);

    const i32 numthreads = i1_min(kMaxThreads, thread_hardware_count());
    ms_numthreads = numthreads;
    ms_worksplit = numthreads * numthreads;

    const i32 kQueueSize = 64;
    ptrqueue_create(ms_queues + 0, EAlloc_Perm, kQueueSize);
    thread_set_aff(NULL, (1ull << 0) | (1ull << 1));
    for (i32 t = 1; t < numthreads; ++t)
    {
        ptrqueue_create(ms_queues + t, EAlloc_Perm, kQueueSize);
        thread_create(ms_threads + t, TaskLoop, NULL);
        thread_set_aff(ms_threads + t, (1ull << t) | (1ull << (t + 1)));
    }
}

ProfileMark(pm_update, TaskSys_Update)
void TaskSys_Update(void)
{
    ProfileBegin(pm_update);

    // clear out backlog, in case thread 0's queue piles up
    const i32 tid = ms_tid;
    while (TryRunTask(tid))
    {

    }

    ProfileEnd(pm_update);
}

void TaskSys_Shutdown(void)
{
    store_i32(&ms_running, 0, MO_Release);
    event_wakeall(&ms_waitDone);
    event_wakeall(&ms_waitPush);
    const i32 numthreads = ms_numthreads;
    for (i32 t = 1; t < numthreads; ++t)
    {
        thread_join(&ms_threads[t]);
        ptrqueue_destroy(&ms_queues[t]);
    }
    ptrqueue_destroy(&ms_queues[0]);

    event_destroy(&ms_waitPush);
    event_destroy(&ms_waitDone);
    intrin_clockres_end(1);

    memset(ms_threads, 0, sizeof(ms_threads));
    memset(ms_queues, 0, sizeof(ms_queues));
    ms_numthreads = 0;
}
