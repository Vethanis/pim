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

typedef struct range_s
{
    i32 begin;
    i32 end;
} range_t;

// ----------------------------------------------------------------------------

static i32 ms_numthreads;
static i32 ms_numThreadsRunning;
static i32 ms_numThreadsSleeping;
static i32 ms_running;
static event_t ms_waitPush;
static thread_t ms_threads[kMaxThreads];
static ptrqueue_t ms_queues[kMaxThreads];

static pim_thread_local i32 ms_tid;

// ----------------------------------------------------------------------------

static i32 StealWork(
    task_t *const pim_noalias task,
    range_t *const pim_noalias range,
    i32 granularity)
{
    const i32 wsize = task->worksize;
    const i32 a = fetch_add_i32(&task->head, granularity, MO_Acquire);
    const i32 b = i1_min(a + granularity, wsize);
    range->begin = a;
    range->end = b;
    return a < b;
}

static i32 UpdateProgress(
    task_t *const pim_noalias task,
    range_t range)
{
    const i32 wsize = task->worksize;
    const i32 count = range.end - range.begin;
    const i32 prev = fetch_add_i32(&task->tail, count, MO_Release);
    ASSERT(prev < wsize);
    return (prev + count) >= wsize;
}

static void MarkComplete(task_t *const pim_noalias task)
{
    store_i32(&task->status, TaskStatus_Complete, MO_Release);
}

static i32 TryRunTask(i32 tid)
{
    const i32 numthreads = ms_numthreads;
    const i32 tasksplit = i1_max(1, numthreads * (numthreads >> 1));
    task_t *const pim_noalias task = ptrqueue_trypop(&ms_queues[tid]);
    if (task)
    {
        const i32 gran = i1_max(1, task->worksize / tasksplit);
        const task_execute_fn fn = task->execute;
        range_t range;
        while (StealWork(task, &range, gran))
        {
            fn(task, range.begin, range.end);
            if (UpdateProgress(task, range))
            {
                MarkComplete(task);
            }
        }
    }
    return task != NULL;
}

static i32 TaskLoop(void* arg)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    inc_i32(&ms_numThreadsRunning, MO_Acquire);

    const i32 tid = (i32)((isize)arg);
    ASSERT(tid);
    ms_tid = tid;

    u64 spins = 0;
    while (load_i32(&ms_running, MO_Relaxed))
    {
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else if (spins < 100)
        {
            intrin_spin(++spins);
        }
        else
        {
            spins = 0;
            inc_i32(&ms_numThreadsSleeping, MO_Acquire);
            event_wait(&ms_waitPush);
            dec_i32(&ms_numThreadsSleeping, MO_Release);
        }
    }

    dec_i32(&ms_numThreadsRunning, MO_Release);

    return 0;
}

// ----------------------------------------------------------------------------

i32 task_thread_id(void)
{
    return ms_tid;
}

i32 task_thread_ct(void)
{
    ASSERT(ms_numthreads > 0);
    return ms_numthreads;
}

i32 task_num_active(void)
{
    return load_i32(&ms_numThreadsRunning, MO_Relaxed) - load_i32(&ms_numThreadsSleeping, MO_Relaxed);
}

TaskStatus task_stat(void const *const pbase)
{
    ASSERT(pbase);
    task_t const *const task = pbase;
    return (TaskStatus)load_i32(&task->status, MO_Acquire);
}

void task_submit(void *const pbase, task_execute_fn execute, i32 worksize)
{
    ASSERT(execute);
    task_t *const task = pbase;
    if (task && worksize > 0)
    {
        ASSERT(task_stat(task) == TaskStatus_Init);
        store_i32(&task->status, TaskStatus_Exec, MO_Release);
        task->execute = execute;
        store_i32(&task->worksize, worksize, MO_Release);
        store_i32(&task->head, 0, MO_Release);
        store_i32(&task->tail, 0, MO_Release);

        const i32 numthreads = ms_numthreads;
        for (i32 t = 0; t < numthreads; ++t)
        {
            if (!ptrqueue_trypush(&ms_queues[t], task))
            {
                INTERRUPT();
            }
        }
    }
}

ProfileMark(pm_await, task_await)
void task_await(const void* pbase)
{
    const task_t* task = pbase;
    if (task)
    {
        ProfileBegin(pm_await);

        const i32 tid = ms_tid;
        u64 spins = 0;
        while (task_stat(task) == TaskStatus_Exec)
        {
            if (TryRunTask(tid))
            {
                spins = 0;
            }
            else
            {
                // trying to wait on an event here sometimes introduces
                // a permanently slept main thread.
                // so instead we do a few spins then yield.
                intrin_spin(++spins);
            }
        }

        ProfileEnd(pm_await);
    }
}

i32 task_poll(const void* pbase)
{
    return task_stat(pbase) != TaskStatus_Exec;
}

void task_run(void* pbase, task_execute_fn fn, i32 worksize)
{
    task_t* task = pbase;
    ASSERT(task);
    ASSERT(fn);
    ASSERT(worksize >= 0);
    if (worksize > 0)
    {
        task_submit(task, fn, worksize);
        task_sys_schedule();
        task_await(task);
    }
}

ProfileMark(pm_schedule, task_sys_schedule)
void task_sys_schedule(void)
{
    ProfileBegin(pm_schedule);

    event_wakeall(&ms_waitPush);

    ProfileEnd(pm_schedule);
}

void task_sys_init(void)
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    intrin_clockres_begin(1);
    event_create(&ms_waitPush);
    store_i32(&ms_running, 1, MO_Release);

    const i32 numthreads = i1_min(kMaxThreads, thread_hardware_count());
    ms_numthreads = numthreads;

    const i32 kQueueSize = 64;
    ptrqueue_create(ms_queues + 0, EAlloc_Perm, kQueueSize);
    thread_set_priority(NULL, thread_priority_highest);
    thread_set_aff(NULL, (1ull << 0) | (1ull << 1));
    for (i32 t = 1; t < numthreads; ++t)
    {
        ptrqueue_create(ms_queues + t, EAlloc_Perm, kQueueSize);
        thread_create(ms_threads + t, TaskLoop, (void*)((isize)t));
        thread_set_priority(ms_threads + t, thread_priority_highest);
        thread_set_aff(ms_threads + t, (1ull << t) | (1ull << (t + 1)));
    }
}

void task_sys_update(void)
{
    // clear out backlog, in case thread 0's queue piles up
    i32 tid = task_thread_id();
    while (TryRunTask(tid))
    {

    }
}

void task_sys_shutdown(void)
{
    store_i32(&ms_running, 0, MO_Release);
    while (load_i32(&ms_numThreadsRunning, MO_Acquire) > 0)
    {
        event_wakeall(&ms_waitPush);
        intrin_yield();
    }
    const i32 numthreads = ms_numthreads;
    for (i32 t = 1; t < numthreads; ++t)
    {
        thread_join(ms_threads + t);
        ptrqueue_destroy(ms_queues + t);
    }
    ptrqueue_destroy(ms_queues + 0);

    event_destroy(&ms_waitPush);
    intrin_clockres_end(1);

    memset(ms_threads, 0, sizeof(ms_threads));
    memset(ms_queues, 0, sizeof(ms_queues));
    ms_numthreads = 0;
}
