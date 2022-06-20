#include "threading/mutex.h"
#include <string.h>

#if PLAT_WINDOWS

#include <Windows.h>

SASSERT(sizeof(Mutex) == sizeof(CRITICAL_SECTION));
SASSERT(_Alignof(Mutex) == _Alignof(CRITICAL_SECTION));

void Mutex_New(Mutex* mut)
{
    ASSERT(mut);
    InitializeCriticalSectionEx((LPCRITICAL_SECTION)mut, 1000, 0);
}

void Mutex_Del(Mutex* mut)
{
    ASSERT(mut);
    DeleteCriticalSection((LPCRITICAL_SECTION)mut);
}

void Mutex_Lock(Mutex* mut)
{
    ASSERT(mut);
    EnterCriticalSection((LPCRITICAL_SECTION)mut);
}

void Mutex_Unlock(Mutex* mut)
{
    ASSERT(mut);
    LeaveCriticalSection((LPCRITICAL_SECTION)mut);
}

bool Mutex_TryLock(Mutex* mut)
{
    ASSERT(mut);
    return TryEnterCriticalSection((LPCRITICAL_SECTION)mut);
}

#else

#include <pthread.h>

CONV_ASSERT(Mutex, pthread_mutex_t);

void Mutex_New(Mutex* mut)
{
    ASSERT(mut);
    i32 rv = pthread_mutex_init((pthread_mutex_t*)mut, NULL);
    ASSERT(!rv);
}

void Mutex_Del(Mutex* mut)
{
    ASSERT(mut);
    i32 rv = pthread_mutex_destroy((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

void Mutex_Lock(Mutex* mut)
{
    ASSERT(mut);
    i32 rv = pthread_mutex_lock((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

void Mutex_Unlock(Mutex* mut)
{
    ASSERT(mut);
    i32 rv = pthread_mutex_unlock((pthread_mutex_t*)mut);
    ASSERT(!rv);
}

bool Mutex_TryLock(Mutex* mut)
{
    ASSERT(mut);
    i32 rv = pthread_mutex_trylock((pthread_mutex_t*)mut);
    return rv == 0;
}
#endif // PLAT
