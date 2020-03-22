#include "threading/mutex.h"

#if PLAT_WINDOWS

#include <Windows.h>

CONV_ASSERT(mutex_t, CRITICAL_SECTION)

void mutex_create(mutex_t* mut)
{
    ASSERT(mut);
    BOOL created = InitializeCriticalSectionAndSpinCount(
        (LPCRITICAL_SECTION)mut, 1000);
    ASSERT(created);
}

void mutex_destroy(mutex_t* mut)
{
    ASSERT(mut);
    DeleteCriticalSection((LPCRITICAL_SECTION)mut);
}

void mutex_lock(mutex_t* mut)
{
    ASSERT(mut);
    EnterCriticalSection((LPCRITICAL_SECTION)mut);
}

void mutex_unlock(mutex_t* mut)
{
    ASSERT(mut);
    LeaveCriticalSection((LPCRITICAL_SECTION)mut);
}

int32_t mutex_trylock(mutex_t* mut)
{
    ASSERT(mut);
    return TryEnterCriticalSection((LPCRITICAL_SECTION)mut);
}

#else

#include <pthread.h>

CONV_ASSERT(mutex_t, pthread_mutex_t)

void mutex_create(mutex_t* mut)
{
    ASSERT(mut);
    int32_t rv = pthread_mutex_init((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

void mutex_destroy(mutex_t* mut)
{
    ASSERT(mut);
    int32_t rv = pthread_mutex_destroy((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

void mutex_lock(mutex_t* mut)
{
    ASSERT(mut);
    int32_t rv = pthread_mutex_lock((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

void mutex_unlock(mutex_t* mut)
{
    ASSERT(mut);
    int32_t rv = pthread_mutex_unlock((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

int32_t mutex_trylock(mutex_t* mut)
{
    ASSERT(mut);
    int32_t rv = pthread_mutex_trylock((pthread_mutex_t*)mut);
    return rv == 0;
}
#endif // PLAT
