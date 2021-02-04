#include "threading/thread.h"
#include "threading/semaphore.h"
#include "allocator/allocator.h"

typedef struct adapter_s
{
    thread_fn entrypoint;
    void* arg;
    void* OShandle;
    semaphore_t sema;
} adapter_t;

#if PLAT_WINDOWS

#include <Windows.h>
#include <process.h>

static HANDLE thread_to_handle(thread_t* tr)
{
    if (tr)
    {
        adapter_t* adapter = tr->handle;
        ASSERT(adapter);
        return adapter->OShandle;
    }
    return GetCurrentThread();
}

static void __cdecl Win32ThreadFn(void* arg)
{
    ASSERT(arg);
    adapter_t* adapter = (adapter_t*)arg;
    adapter->entrypoint(adapter->arg);
    semaphore_signal(adapter->sema, 1);
    _endthread();
}

void thread_create(thread_t* tr, thread_fn entrypoint, void* arg)
{
    ASSERT(tr);
    ASSERT(entrypoint);

    adapter_t* adapter = perm_calloc(sizeof(*adapter));
    adapter->entrypoint = entrypoint;
    adapter->arg = arg;
    semaphore_create(&(adapter->sema), 0);

    uintptr_t handle = _beginthread(
        Win32ThreadFn,
        0,
        adapter);
    ASSERT(handle);

    adapter->OShandle = (void*)handle;

    tr->handle = adapter;
}

void thread_join(thread_t* tr)
{
    ASSERT(tr);
    adapter_t* adapter = tr->handle;
    ASSERT(adapter);

    semaphore_wait(adapter->sema);
    semaphore_destroy(&(adapter->sema));

    pim_free(adapter);
    tr->handle = NULL;
}

void thread_set_aff(thread_t* tr, u64 mask)
{
    ASSERT(mask);
    HANDLE hThread = thread_to_handle(tr);
    DWORD_PTR rval = SetThreadAffinityMask(hThread, mask);
    ASSERT(rval != 0);
}

void thread_set_priority(thread_t* tr, i32 priority)
{
    i32 threadpriority = THREAD_PRIORITY_NORMAL;
    i32 procpriority = NORMAL_PRIORITY_CLASS;
    switch (priority)
    {
    default:
        ASSERT(false);
        return;
    case -2:
        threadpriority = THREAD_PRIORITY_LOWEST;
        procpriority = IDLE_PRIORITY_CLASS;
        break;
    case -1:
        threadpriority = THREAD_PRIORITY_BELOW_NORMAL;
        procpriority = BELOW_NORMAL_PRIORITY_CLASS;
        break;
    case 0:
        threadpriority = THREAD_PRIORITY_NORMAL;
        procpriority = NORMAL_PRIORITY_CLASS;
        break;
    case 1:
        threadpriority = THREAD_PRIORITY_ABOVE_NORMAL;
        procpriority = ABOVE_NORMAL_PRIORITY_CLASS;
        break;
    case 2:
        threadpriority = THREAD_PRIORITY_HIGHEST;
        procpriority = HIGH_PRIORITY_CLASS;
        break;
    }

    if (!tr)
    {
        HANDLE hProcess = GetCurrentProcess();
        bool set = SetPriorityClass(hProcess, procpriority);
        ASSERT(set);
    }

    HANDLE hThread = thread_to_handle(tr);
    ASSERT(hThread);
    if (hThread)
    {
        bool set = SetThreadPriority(hThread, threadpriority);
        ASSERT(set);
    }
}

i32 thread_hardware_count(void)
{
    SYSTEM_INFO systeminfo = { 0 };
    GetSystemInfo(&systeminfo);
    i32 count = systeminfo.dwNumberOfProcessors;
    ASSERT(count > 0);
    count = (count > 0) ? count : 1;
    count = (count < kMaxThreads) ? count : kMaxThreads;
    return count;
}

#else

#include <pthread.h>

SASSERT(sizeof(pthread_t) == sizeof(thread_t));
SASSERT(alignof(pthread_t) == alignof(thread_t));

void thread_create(thread_t* tr, thread_fn entrypoint, void* arg)
{
    ASSERT(tr);
    tr->handle = NULL;
    pthread_t* pt = (pthread_t*)tr;
    i32 rv = pthread_create(pt, NULL, entrypoint, arg);
    ASSERT(!rv);
}

void thread_join(thread_t* tr)
{
    ASSERT(tr);
    pthread_t* pt = (pthread_t*)tr;
    i32 rv = pthread_join(pt, NULL);
    ASSERT(!rv);
    tr->handle = NULL;
}

#endif // PLAT
