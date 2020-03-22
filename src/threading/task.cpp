#include "threading/task.h"
#include "os/thread.h"
#include "common/atomics.h"
#include "containers/ptrqueue.h"
#include "common/random.h"
#include <string.h>

#define kNumThreads     32
#define kTaskSplit      (kNumThreads * (kNumThreads / 2))

typedef struct task_s
{
    task_execute_fn execute;
    void* data;
    int32_t status;
    int32_t awaits;
    int32_t worksize;
    int32_t head;
    int32_t tail;
} task_t;

typedef struct range_s
{
    int32_t begin;
    int32_t end;
} range_t;

// ----------------------------------------------------------------------------

static OS::Thread ms_threads[kNumThreads];
static PtrQueue ms_queues[kNumThreads];

static OS::Event ms_waitPush;
static OS::Event ms_waitExec;

static int32_t ms_numThreadsRunning;
static int32_t ms_numThreadsSleeping;
static int32_t ms_running;

static PIM_TLS int32_t ms_tid;

// ----------------------------------------------------------------------------

static int32_t min_i32(int32_t a, int32_t b) { return (a < b) ? a : b; }
static int32_t max_i32(int32_t a, int32_t b) { return (a > b) ? a : b; }

static range_t StealWork(task_t* task, int32_t worksize, int32_t granularity)
{
    range_t range;
    range.begin = fetch_add_i32(&(task->head), granularity, MO_Acquire);
    range.end = min_i32(range.begin + granularity, worksize);
    return range;
}

static int32_t UpdateProgress(task_t* task, range_t range)
{
    int32_t count = range.end - range.begin;
    int32_t size = load_i32(&(task->worksize), MO_Relaxed);
    int32_t prev = fetch_add_i32(&(task->tail), count, MO_AcqRel);
    ASSERT(prev < size);
    return (prev + count) >= size;
}

static void MarkComplete(task_t* task)
{
    store_i32(&(task->status), TaskStatus_Complete, MO_Release);
    while (load_i32(&(task->awaits), MO_Acquire) > 0)
    {
        ms_waitExec.WakeAll();
        OS::SwitchThread();
    }
}

static int32_t TryRunTask(int32_t tid)
{
    task_t* task = (task_t*)(ms_queues[tid].TryPop());
    if (task)
    {
        const int32_t worksize = task->worksize;
        const int32_t granularity = max_i32(1, worksize / kTaskSplit);
        range_t range;

    stealloop:
        range = StealWork(task, worksize, granularity);
        if (range.begin < range.end)
        {
            task->execute(task->data, range.begin, range.end);

            if (UpdateProgress(task, range))
            {
                MarkComplete(task);
            }
            else
            {
                goto stealloop;
            }
        }
    }

    return task != 0;
}

static void TaskLoop(void* pVoid)
{
    inc_i32(&ms_numThreadsRunning, MO_Acquire);

    Random::Seed();

    ms_tid = (int32_t)((isize)pVoid);
    const int32_t tid = ms_tid;
    ASSERT(tid);

    while (load_i32(&ms_running, MO_Relaxed))
    {
        if (!TryRunTask(tid))
        {
            inc_i32(&ms_numThreadsSleeping, MO_Acquire);
            ms_waitPush.Wait();
            dec_i32(&ms_numThreadsSleeping, MO_Release);
        }
    }

    dec_i32(&ms_numThreadsRunning, MO_Release);
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

TaskStatus task_stat(task_hdl hdl)
{
    task_t* task = (task_t*)hdl;
    ASSERT(task);
    return (TaskStatus)load_i32(&(task->status), MO_Acquire);
}

task_hdl task_submit(void* data, task_execute_fn execute, int32_t worksize)
{
    ASSERT(execute);
    ASSERT(worksize > 0);

    task_t* task = pim_tcalloc(task_t, EAlloc_TLS, 1);
    ASSERT(task);

    task->execute = execute;
    task->data = data;
    task->status = TaskStatus_Exec;
    task->worksize = worksize;

    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        ms_queues[t].Push(task);
    }

    ms_waitPush.WakeAll();

    return task;
}

void* task_complete(task_hdl* pHandle)
{
    void* result = 0;

    ASSERT(pHandle);
    void* hdl = *pHandle;
    *pHandle = 0;
    task_t* task = (task_t*)hdl;

    if (task)
    {
        inc_i32(&(task->awaits), MO_Acquire);
        while (task_stat(task) != TaskStatus_Complete)
        {
            ms_waitExec.Wait();
        }
        dec_i32(&(task->awaits), MO_Release);

        result = task->data;
        pim_free(task);
    }

    return result;
}

void task_sys_init(void)
{
    ms_waitPush.Open();
    ms_waitExec.Open();
    store_i32(&ms_running, 1, MO_Release);
    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        ms_queues[t].Init(EAlloc_Perm, 256);
        ms_threads[t].Open(TaskLoop, (void*)((isize)t));
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
        ms_waitPush.WakeAll();
        ms_waitExec.WakeAll();
        OS::SwitchThread();
    }
    for (int32_t t = 1; t < kNumThreads; ++t)
    {
        ms_threads[t].Join();
        ms_queues[t].Reset();
    }
    ms_waitPush.Close();
    ms_waitExec.Close();
}
