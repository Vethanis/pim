#include "threading/thread.h"
#include "threading/semaphore.h"
#include "allocator/allocator.h"

#if PLAT_WINDOWS

typedef struct adapter_s
{
    i32(PIM_CDECL *entrypoint)(void*);
    void* arg;
    void* OShandle;
    Semaphore sema;
} adapter_t;

typedef void* (*pthread_fn)(void*);


#include <Windows.h>
#include <process.h>

static HANDLE thread_to_handle(Thread* tr)
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
    Semaphore_Signal(adapter->sema, 1);
    _endthread();
}

void Thread_New(Thread* tr, i32(PIM_CDECL *entrypoint)(void*), void* arg)
{
    ASSERT(tr);
    ASSERT(entrypoint);

    adapter_t* adapter = Perm_Calloc(sizeof(*adapter));
    adapter->entrypoint = entrypoint;
    adapter->arg = arg;
    Semaphore_New(&(adapter->sema), 0);

    uintptr_t handle = _beginthread(
        Win32ThreadFn,
        0,
        adapter);
    ASSERT(handle);

    adapter->OShandle = (void*)handle;

    tr->handle = adapter;
}

void Thread_Join(Thread* tr)
{
    ASSERT(tr);
    adapter_t* adapter = tr->handle;
    ASSERT(adapter);

    Semaphore_Wait(adapter->sema);
    Semaphore_Del(&(adapter->sema));

    Mem_Free(adapter);
    tr->handle = NULL;
}

void Thread_SetAffinity(Thread* tr, u64 mask)
{
    ASSERT(mask);
    HANDLE hThread = thread_to_handle(tr);
    DWORD_PTR rval = SetThreadAffinityMask(hThread, mask);
    ASSERT(rval != 0);
}

void Thread_SetPriority(Thread* tr, ThreadPriority priority)
{
    i32 threadpriority = THREAD_PRIORITY_NORMAL;
    i32 procpriority = NORMAL_PRIORITY_CLASS;
    switch (priority)
    {
    default:
        ASSERT(false);
        return;
    case ThreadPriority_Lowest:
        threadpriority = THREAD_PRIORITY_LOWEST;
        procpriority = IDLE_PRIORITY_CLASS;
        break;
    case ThreadPriority_Lower:
        threadpriority = THREAD_PRIORITY_BELOW_NORMAL;
        procpriority = BELOW_NORMAL_PRIORITY_CLASS;
        break;
    case ThreadPriority_Normal:
        threadpriority = THREAD_PRIORITY_NORMAL;
        procpriority = NORMAL_PRIORITY_CLASS;
        break;
    case ThreadPriority_Higher:
        threadpriority = THREAD_PRIORITY_ABOVE_NORMAL;
        procpriority = ABOVE_NORMAL_PRIORITY_CLASS;
        break;
    case ThreadPriority_Highest:
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

i32 Thread_HardwareCount(void)
{
    SYSTEM_INFO systeminfo = { 0 };
    GetSystemInfo(&systeminfo);
    i32 count = systeminfo.dwNumberOfProcessors;
    ASSERT(count > 0);
    count = pim_max(count, 1);
    count = pim_min(count, kMaxThreads);
    return count;
}

#else

typedef struct PthreadArgs_s
{
    i32(*func)(void*);
    void* arg;
} PthreadArgs;

static void* PthreadAdapterFn(void* voidArg)
{
    PthreadArgs* args = voidArg;
    args->func(args->arg);
    Mem_Free(args);
    return NULL;
}

typedef void* (*pthread_fn)(void*);

#include <sys/sysinfo.h>
#include <pthread.h>

SASSERT(sizeof(pthread_t) == sizeof(Thread));
SASSERT(pim_alignof(pthread_t) == pim_alignof(Thread));

void Thread_New(Thread* tr, i32(PIM_CDECL *entrypoint)(void*), void* arg)
{
    ASSERT(tr);
    tr->handle = NULL;
    pthread_t* pt = (pthread_t*)tr;
    PthreadArgs* args = Perm_Calloc(sizeof(PthreadArgs));
    args->func = entrypoint;
    args->arg = arg;
    i32 rv = pthread_create(pt, NULL, PthreadAdapterFn, args);
    ASSERT(!rv);
}

void Thread_Join(Thread* tr)
{
    ASSERT(tr);
    pthread_t* pt = (pthread_t*)tr;
    i32 rv = pthread_join(*pt, NULL);
    ASSERT(!rv);
    tr->handle = NULL;
}

i32 Thread_HardwareCount(void)
{
    return get_nprocs();
}

#endif // PLAT
