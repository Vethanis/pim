#include "threading/semaphore.h"
#include "allocator/allocator.h"

#if PLAT_WINDOWS

#include <Windows.h>

CONV_ASSERT(semaphore_t, HANDLE)

void semaphore_create(semaphore_t* sema, int32_t value)
{
    ASSERT(sema);
    HANDLE handle = CreateSemaphoreA(NULL, value, 0x7fffffff, NULL);
    ASSERT(handle);
    sema->handle = handle;
}

void semaphore_destroy(semaphore_t* sema)
{
    ASSERT(sema);
    HANDLE handle = sema->handle;
    sema->handle = NULL;
    ASSERT(handle);
    BOOL closed = CloseHandle(handle);
    ASSERT(closed);
}

void semaphore_signal(semaphore_t sema, int32_t count)
{
    ASSERT(sema.handle);
    ASSERT(count >= 0);
    BOOL released = ReleaseSemaphore(sema.handle, count, NULL);
    ASSERT(released);
}

void semaphore_wait(semaphore_t sema)
{
    ASSERT(sema.handle);
    DWORD status = WaitForSingleObject(sema.handle, INFINITE);
    ASSERT(status == WAIT_OBJECT_0);
}

int32_t semaphore_trywait(semaphore_t sema)
{
    ASSERT(sema.handle);
    DWORD status = WaitForSingleObject(sema.handle, 0);
    ASSERT((status == WAIT_OBJECT_0) || (status == WAIT_TIMEOUT));
    return status == WAIT_OBJECT_0;
}

#else

#include <semaphore.h>

// sem_wait is interruptable, must check for EINTR
static int32_t sem_wait_safe(sem_t* sem)
{
    while (sem_wait(sem))
    {
        if (errno == EINTR)
        {
            errno = 0;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

// sem_trywait is interruptable, must check for EINTR
static int32_t sem_trywait_safe(sem_t* sem)
{
    while (sem_trywait(sem))
    {
        if (errno == EINTR)
        {
            errno = 0;
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

void semaphore_create(semaphore_t* sema, int32_t value)
{
    ASSERT(sema);
    sem_t* handle = perm_malloc(sizeof(sem_t));
    int32_t rv = sem_init(handle, 0, value);
    ASSERT(!rv);
    sema->handle = handle;
}

void semaphore_destroy(semaphore_t* sema)
{
    ASSERT(sema);
    sem_t* handle = sema->handle;
    sema->handle = NULL;
    ASSERT(handle);
    int32_t rv = sem_destroy(handle);
    ASSERT(!rv);
    pim_free(handle);
}

void semaphore_signal(semaphore_t sema, int32_t count)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    for (int32_t i = 0; i < count; ++i)
    {
        int32_t rv = sem_post(handle);
        ASSERT(!rv);
    }
}

void semaphore_wait(semaphore_t sema)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    int32_t rv = sem_wait_safe(handle);
    ASSERT(!rv);
}

int32_t semaphore_trywait(semaphore_t sema)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    int32_t rv = sem_trywait_safe(handle);
    return rv == 0;
}


#endif // PLAT
