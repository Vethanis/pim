#include "threading/semaphore.h"
#include "allocator/allocator.h"

#if PLAT_WINDOWS

#include <Windows.h>

SASSERT(sizeof(semaphore_t) >= sizeof(HANDLE));
SASSERT(_Alignof(semaphore_t) >= _Alignof(HANDLE));

void semaphore_create(semaphore_t* sema, i32 value)
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
    if (handle)
    {
        BOOL closed = CloseHandle(handle);
        ASSERT(closed);
    }
}

void semaphore_signal(semaphore_t sema, i32 count)
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

bool semaphore_trywait(semaphore_t sema)
{
    ASSERT(sema.handle);
    DWORD status = WaitForSingleObject(sema.handle, 0);
    ASSERT((status == WAIT_OBJECT_0) || (status == WAIT_TIMEOUT));
    return status == WAIT_OBJECT_0;
}

#else

#include <semaphore.h>

// sem_wait is interruptable, must check for EINTR
static i32 sem_wait_safe(sem_t* sem)
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
static i32 sem_trywait_safe(sem_t* sem)
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

void semaphore_create(semaphore_t* sema, i32 value)
{
    ASSERT(sema);
    sem_t* handle = Perm_Alloc(sizeof(sem_t));
    i32 rv = sem_init(handle, 0, value);
    ASSERT(!rv);
    sema->handle = handle;
}

void semaphore_destroy(semaphore_t* sema)
{
    ASSERT(sema);
    sem_t* handle = sema->handle;
    sema->handle = NULL;
    ASSERT(handle);
    i32 rv = sem_destroy(handle);
    ASSERT(!rv);
    Mem_Free(handle);
}

void semaphore_signal(semaphore_t sema, i32 count)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    for (i32 i = 0; i < count; ++i)
    {
        i32 rv = sem_post(handle);
        ASSERT(!rv);
    }
}

void semaphore_wait(semaphore_t sema)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    i32 rv = sem_wait_safe(handle);
    ASSERT(!rv);
}

bool semaphore_trywait(semaphore_t sema)
{
    sem_t* handle = sema.handle;
    ASSERT(handle);
    i32 rv = sem_trywait_safe(handle);
    return rv == 0;
}


#endif // PLAT
