#include "threading/semaphore.h"
#include "allocator/allocator.h"

#if PLAT_WINDOWS

#include <Windows.h>

CONV_ASSERT(semaphore_t, HANDLE)

semaphore_t semaphore_create(int32_t value)
{
    HANDLE handle = CreateSemaphoreA(NULL, value, 0x7fffffff, NULL);
    ASSERT(handle);
    return handle;
}

void semaphore_destroy(semaphore_t sema)
{
    ASSERT(sema);
    BOOL closed = CloseHandle(sema);
    ASSERT(closed);
}

void semaphore_signal(semaphore_t sema, int32_t count)
{
    ASSERT(sema);
    ASSERT(count >= 0);
    BOOL released = ReleaseSemaphore(sema, count, NULL);
    ASSERT(released);
}

void semaphore_wait(semaphore_t sema)
{
    ASSERT(sema);
    DWORD status = WaitForSingleObject(sema, INFINITE);
    ASSERT(status == WAIT_OBJECT_0);
}

int32_t semaphore_trywait(semaphore_t sema)
{
    ASSERT(sema);
    DWORD status = WaitForSingleObject(sema, 0);
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

semaphore_t semaphore_create(int32_t value)
{
    sem_t* sema = pim_malloc(EAlloc_Perm, sizeof(sem_t));
    ASSERT(sema);
    int32_t rv = sem_init(sema, 0, value);
    ASSERT(!rv);
    return sema;
}

void semaphore_destroy(semaphore_t sema)
{
    ASSERT(sema);
    int32_t rv = sem_destroy(sema);
    ASSERT(!rv);
    pim_free(sema);
}

void semaphore_signal(semaphore_t sema, int32_t count)
{
    ASSERT(sema);
    for (int32_t i = 0; i < count; ++i)
    {
        int32_t rv = sem_post(sema);
        ASSERT(!rv);
    }
}

void semaphore_wait(semaphore_t sema)
{
    ASSERT(sema);
    int32_t rv = sem_wait_safe(sema);
    ASSERT(!rv);
}

int32_t semaphore_trywait(semaphore_t sema)
{
    ASSERT(sema);
    int32_t rv = sem_trywait_safe(sema);
    return rv == 0;
}


#endif // PLAT
