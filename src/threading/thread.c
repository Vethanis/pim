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

thread_t thread_create(thread_fn entrypoint, void* arg)
{
    ASSERT(entrypoint);

    adapter_t* adapter = pim_malloc(EAlloc_Perm, sizeof(adapter_t));
    ASSERT(adapter);
    adapter->entrypoint = entrypoint;
    adapter->arg = arg;
    adapter->sema = semaphore_create(0);

    uintptr_t handle = _beginthread(
        Win32ThreadFn,
        0,
        adapter);
    ASSERT(handle);

    return adapter;
}

void thread_join(thread_t tr)
{
    ASSERT(tr);
    adapter_t* adapter = (adapter_t*)tr;
    semaphore_wait(adapter->sema);
    semaphore_destroy(adapter->sema);
    pim_free(adapter);
}

#else

#include <pthread.h>

SASSERT(sizeof(pthread_t) == sizeof(thread_t));
SASSERT(alignof(pthread_t) == alignof(thread_t));

thread_t thread_create(thread_fn entrypoint, void* arg)
{
    pthread_t tr = NULL;
    int32_t rv = pthread_create(&tr, NULL, entrypoint, arg);
    ASSERT(!rv);
    return tr;
}

void thread_join(thread_t tr)
{
    ASSERT(tr);
    int32_t rv = pthread_join(tr, NULL);
    ASSERT(!rv);
}

#endif // PLAT
