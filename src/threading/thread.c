#include "threading/thread.h"
#include "threading/semaphore.h"
#include "allocator/allocator.h"
#include <string.h>

typedef struct adapter_s
{
    thread_fn entrypoint;
    void* arg;
    semaphore_t sema;
} adapter_t;

#if PLAT_WINDOWS

#include <process.h>

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

    adapter_t* adapter = pim_malloc(EAlloc_Perm, sizeof(adapter_t));
    adapter->entrypoint = entrypoint;
    adapter->arg = arg;
    semaphore_create(&(adapter->sema), 0);

    uintptr_t handle = _beginthread(
        Win32ThreadFn,
        0,
        adapter);
    ASSERT(handle);

    tr->handle = adapter;
}

void thread_join(thread_t* tr)
{
    ASSERT(tr);
    adapter_t* adapter = tr->handle;
    semaphore_wait(adapter->sema);
    semaphore_destroy(&(adapter->sema));
    pim_free(adapter);
    tr->handle = NULL;
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
    int32_t rv = pthread_create(pt, NULL, entrypoint, arg);
    ASSERT(!rv);
}

void thread_join(thread_t* tr)
{
    ASSERT(tr);
    pthread_t* pt = (pthread_t*)tr;
    int32_t rv = pthread_join(pt, NULL);
    ASSERT(!rv);
    tr->handle = NULL;
}

#endif // PLAT
