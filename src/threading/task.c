#include "threading/task.h"

#include "threading/thread.h"
#include "threading/event.h"
#include "threading/intrin.h"
#include "common/atomics.h"
#include "common/random.h"
#include "containers/ptrqueue.h"

#include <string.h>

#define kNumThreads     32
#define kTaskSplit      (kNumThreads * (kNumThreads / 2))

typedef struct range_s
{
    int32_t begin;
    int32_t end;
} range_t;

// ----------------------------------------------------------------------------

static thread_t ms_threads[kNumThreads];
static ptrqueue_t ms_queues[kNumThreads];

static event_t ms_waitPush;
static event_t ms_waitExec;

static int32_t ms_numThreadsRunning;
static int32_t ms_numThreadsSleeping;
static int32_t ms_running;

static PIM_TLS int32_t ms_tid;

// ----------------------------------------------------------------------------

static int32_t min_i32(int32_t a, int32_t b) { return (a < b) ? a : b; }
static int32_t max_i32(int32_t a, int32_t b) { return (a > b) ? a : b; }

static int32_t StealWork(task_t* task, range_t* range)
{
    const int32_t gran = task->granularity;
    const int32_t wsize = task->worksize;
    const int32_t a = fetch_add_i32(&(task->head), gran, MO_Acquire);
    const int32_t b = min_i32(a + gran, wsize);
    range->begin = a;
    range->end = b;
    return a < b;
}

static int32_t UpdateProgress(task_t* task, range_t range)
{
    const int32_t wsize = task->worksize;
    const int32_t count = range.end - range.begin;
    const int32_t prev = fetch_add_i32(&(task->tail), count, MO_AcqRel);
    ASSERT(prev < wsize);
    return (prev + count) >= wsize;
}

static void MarkComplete(task_t* task)
{
    store_i32(&(task->status), TaskStatus_Complete, MO_Release);
    while (load_i32(&(task->awaits), MO_Acquire) > 0)
    {
        event_wakeall(&ms_waitExec);
        intrin_yield();
    }
}

static int32_t TryRunTask(int32_t tid)
{
    task_t* task = ptrqueue_trypop(ms_queues + tid);
    if (task)
    {
        range_t range;
        while (StealWork(task, &range))
        {
            task->execute(task, range.begin, range.end);
            if (UpdateProgress(task, range))
            {
                MarkComplete(task);
            }
        }
    }
    return task != NULL;
}

static int32_t TaskLoop(void* arg)
{
    inc_i32(&ms_numThreadsRunning, MO_Acquire);

    rand_autoseed();

    const int32_t tid = (int32_t)((intptr_t)arg);
    ASSERT(tid);
    ms_tid = tid;

    int64_t spins = 0;
    while (load_i32(&ms_running, MO_Relaxed))
    {
        if (TryRunTask(tid))
        {
            spins = 0;
        }
        else
        {
            if (spins < 3)
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
    }

    dec_i32(&ms_numThreadsRunning, MO_Release);

    return 0;
}

// ----------------------------------------------------------------------------

int32_t task_thread_id(void)
{
    return ms_tid;
}

int32_t task_num_active(void)
{
    return load_i32(&ms_numThreadsRunning, MO_Relaxed) - load_i32(&ms_numThreadsSleeping, MO_Relaxed);
}

TaskStatus task_stat(task_t* task)
{
    ASSERT(task);
    return (TaskStatus)load_i32(&(task->status), MO_Acquire);
}

void task_submit(task_t* task, task_execute_fn execute, int32_t worksize)
{
    ASSERT(execute);
    ASSERT(worksize > 0);
    ASSERT(task_stat(task) != TaskStatus_Exec);
    ASSERT(load_i32(&(task->awaits), MO_Relaxed) == 0);

    store_i32(&(task->status), TaskStatus_Exec, MO_Release);
    task->execute = execute;
    store_i32(&(task->worksize), worksize, MO_Release);
    store_i32(&(task->granularity), max_i32(1, worksize / kTaskSplit), MO_Release);
    store_i32(&(task->head), 0, MO_Release);
    store_i32(&(task->tail), 0, MO_Release);

    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        ptrqueue_push(ms_queues + t, task);
    }

    event_wakeall(&ms_waitPush);
}

void task_await(task_t* task)
{
    ASSERT(task);
    inc_i32(&(task->awaits), MO_Acquire);
    while (task_stat(task) == TaskStatus_Exec)
    {
        event_wait(&ms_waitExec);
    }
    dec_i32(&(task->awaits), MO_Release);
}

void task_sys_init(void)
{
    event_create(&ms_waitPush);
    event_create(&ms_waitExec);
    store_i32(&ms_running, 1, MO_Release);
    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        ptrqueue_create(ms_queues + t, EAlloc_Perm, 256);
        thread_create(ms_threads + t, TaskLoop, (void*)((intptr_t)t));
    }
}

void task_sys_update(void)
{

}

void task_sys_shutdown(void)
{
    store_i32(&ms_running, 0, MO_Release);
    while (load_i32(&ms_numThreadsRunning, MO_Acquire) > 0)
    {
        event_wakeall(&ms_waitPush);
        event_wakeall(&ms_waitExec);
        intrin_yield();
    }
    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        thread_join(ms_threads + t);
        ptrqueue_destroy(ms_queues + t);
    }
    memset(ms_threads, 0, sizeof(ms_threads));
    memset(ms_queues, 0, sizeof(ms_queues));
    event_destroy(&ms_waitPush);
    event_destroy(&ms_waitExec);
}
