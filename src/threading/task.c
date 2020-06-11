#include "threading/task.h"

#include "threading/thread.h"
#include "threading/event.h"
#include "threading/intrin.h"
#include "threading/sleep.h"
#include "common/atomics.h"
#include "containers/ptrqueue.h"
#include "common/profiler.h"

#include <string.h>

#define kTaskSplit      (kNumThreads * (kNumThreads / 2))

typedef struct range_s
{
    i32 begin;
    i32 end;
} range_t;

// ----------------------------------------------------------------------------

static thread_t ms_threads[kNumThreads];
static ptrqueue_t ms_queues[kNumThreads];

static event_t ms_waitPush;

static i32 ms_numThreadsRunning;
static i32 ms_numThreadsSleeping;
static i32 ms_running;

static pim_thread_local i32 ms_tid;

// ----------------------------------------------------------------------------

static i32 min_i32(i32 a, i32 b) { return (a < b) ? a : b; }
static i32 max_i32(i32 a, i32 b) { return (a > b) ? a : b; }

static i32 StealWork(task_t* task, range_t* range, i32 gran)
{
    const i32 wsize = task->worksize;
    const i32 a = fetch_add_i32(&(task->head), gran, MO_Acquire);
    const i32 b = min_i32(a + gran, wsize);
    range->begin = a;
    range->end = b;
    return a < b;
}

static i32 UpdateProgress(task_t* task, range_t range)
{
    const i32 wsize = task->worksize;
    const i32 count = range.end - range.begin;
    const i32 prev = fetch_add_i32(&(task->tail), count, MO_Release);
    ASSERT(prev < wsize);
    return (prev + count) >= wsize;
}

static void MarkComplete(task_t* task)
{
    store_i32(&(task->status), TaskStatus_Complete, MO_Release);
}

static i32 TryRunTask(i32 tid)
{
    task_t* task = ptrqueue_trypop(ms_queues + tid);
    if (task)
    {
        const i32 gran = max_i32(1, task->worksize / kTaskSplit);
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
    inc_i32(&ms_numThreadsRunning, MO_Acquire);

    const i32 tid = (i32)((isize)arg);
    ASSERT(tid);
    ms_tid = tid;

    while (load_i32(&ms_running, MO_Relaxed))
    {
        if (!TryRunTask(tid))
        {
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

i32 task_num_active(void)
{
    return load_i32(&ms_numThreadsRunning, MO_Relaxed) - load_i32(&ms_numThreadsSleeping, MO_Relaxed);
}

TaskStatus task_stat(const task_t* task)
{
    ASSERT(task);
    return (TaskStatus)load_i32(&(task->status), MO_Acquire);
}

void task_submit(task_t* task, task_execute_fn execute, i32 worksize)
{
    if (task && worksize > 0)
    {
        ASSERT(execute);
        task_await(task);
        store_i32(&(task->status), TaskStatus_Exec, MO_Release);
        task->execute = execute;
        store_i32(&(task->worksize), worksize, MO_Release);
        store_i32(&(task->head), 0, MO_Release);
        store_i32(&(task->tail), 0, MO_Release);

        for (i32 t = 0; t < kNumThreads; ++t)
        {
            if (!ptrqueue_trypush(ms_queues + t, task))
            {
                ASSERT(false);
            }
        }
    }
}

ProfileMark(pm_await, task_await)
void task_await(const task_t* task)
{
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

i32 task_poll(const task_t* task)
{
    return task_stat(task) != TaskStatus_Exec;
}

void task_run(task_t* task, task_execute_fn fn, i32 worksize)
{
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
    intrin_clockres_begin(1);
    event_create(&ms_waitPush);
    store_i32(&ms_running, 1, MO_Release);

    ptrqueue_create(ms_queues + 0, EAlloc_Perm, 32);
    for (i32 t = 1; t < kNumThreads; ++t)
    {
        ptrqueue_create(ms_queues + t, EAlloc_Perm, 32);
        thread_create(ms_threads + t, TaskLoop, (void*)((isize)t));
    }
}

ProfileMark(pm_update, task_sys_update)
void task_sys_update(void)
{
    ProfileBegin(pm_update);

    task_sys_schedule();

    ProfileEnd(pm_update);
}

void task_sys_shutdown(void)
{
    store_i32(&ms_running, 0, MO_Release);
    while (load_i32(&ms_numThreadsRunning, MO_Acquire) > 0)
    {
        event_wakeall(&ms_waitPush);
        intrin_yield();
    }
    for (i32 t = 1; t < kNumThreads; ++t)
    {
        thread_join(ms_threads + t);
        ptrqueue_destroy(ms_queues + t);
    }
    ptrqueue_destroy(ms_queues + 0);

    memset(ms_threads, 0, sizeof(ms_threads));
    memset(ms_queues, 0, sizeof(ms_queues));
    event_destroy(&ms_waitPush);
    intrin_clockres_end(1);
}
