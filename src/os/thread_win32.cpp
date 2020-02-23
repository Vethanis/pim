#include "common/macro.h"

#if PLAT_WINDOWS

#include <windows.h>

#include "os/atomics.h"
#include "os/thread.h"
#include <intrin.h>

// for WaitOnAddress
#pragma comment(lib, "synchronization.lib")

void OS::YieldCore()
{
    _mm_pause();
}

u64 OS::ReadCounter()
{
    return __rdtsc();
}

void OS::Spin(u64 ticks)
{
    ticks *= 100;
    if (ticks >= 1000)
    {
        ::SwitchToThread();
    }
    else
    {
        const u64 end = __rdtsc() + ticks;
        do
        {
            _mm_pause();
        } while (__rdtsc() < end);
    }
}

void OS::Rest(u64 ms)
{
    ::Sleep((u32)ms);
}

bool OS::SwitchThread()
{
    return ::SwitchToThread();
}

// ------------------------------------------------------------------------

struct ThreadAdapter
{
    OS::ThreadFn fn;
    void* data;
};

static constexpr isize MaxThreads = 64;
static ThreadAdapter ms_adapters[MaxThreads];
static i32 ms_adapterLocks[MaxThreads];

static isize AllocAdapter(ThreadAdapter adapter)
{
    u64 spins = 0;
    while (spins < 10)
    {
        for (isize i = 0; i < MaxThreads; ++i)
        {
            i32 state = 0;
            if (CmpExStrong(ms_adapterLocks[i], state, 1, MO_Acquire, MO_Relaxed))
            {
                ms_adapters[i].fn = adapter.fn;
                ms_adapters[i].data = adapter.data;
                return i;
            }
        }
        OS::Spin(++spins);
    }
    ASSERT(false);
    return -1;
}

static void FreeAdapter(isize i)
{
    Store(ms_adapterLocks[i], 0, MO_Release);
}

static unsigned long Win32ThreadFn(void* pVoid)
{
    const isize tid = (isize)pVoid;
    ASSERT((usize)tid < (usize)MaxThreads);

    OS::ThreadFn fn = ms_adapters[tid].fn;
    void* pData = ms_adapters[tid].data;
    ms_adapters[tid].data = NULL;
    ms_adapters[tid].fn = NULL;

    FreeAdapter(tid);

    ASSERT(fn != NULL);
    fn(pData);

    return 0;
}

// ------------------------------------------------------------------------

namespace OS
{
    bool Thread::Open(ThreadFn fn, void* data)
    {
        ASSERT(fn);
        ASSERT(!handle);

        const isize tid = AllocAdapter({ fn, data });
        ASSERT(tid != -1);

        // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
        handle = ::CreateThread(
            nullptr,
            0,
            Win32ThreadFn,
            (void*)tid,
            0,
            nullptr);

        ASSERT(handle != NULL);
        return handle != NULL;
    }

    bool Thread::Join()
    {
        if (handle)
        {
            // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
            u32 status = ::WaitForSingleObject(handle, INFINITE);
            ASSERT(!status);

            // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
            bool closed = ::CloseHandle(handle);
            handle = nullptr;
            ASSERT(closed);
            return true;
        }
        return false;
    }

    // ----------------------------------------------------------------------------

    bool Semaphore::Open(u32 initialValue)
    {
        constexpr u32 MaxValue = 0x7FFFFFFF;
        ASSERT(initialValue <= MaxValue);
        ASSERT(!handle);
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsemaphorea
        handle = ::CreateSemaphoreA(nullptr, initialValue, MaxValue, nullptr);
        ASSERT(handle);
        return handle != NULL;
    }

    bool Semaphore::Close()
    {
        if (handle)
        {
            // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
            bool closed = ::CloseHandle(handle);
            ASSERT(closed);
            handle = nullptr;
            return closed;
        }
        return false;
    }

    bool Semaphore::Signal(u32 count)
    {
        ASSERT(handle);
        // increment by count
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
        bool rval = ::ReleaseSemaphore(handle, count, nullptr);
        ASSERT(rval);
        return rval;
    }

    bool Semaphore::Wait()
    {
        ASSERT(handle);
        // decrement by one
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
        u32 status = ::WaitForSingleObject(handle, INFINITE);
        ASSERT(status == WAIT_OBJECT_0);
        return status == WAIT_OBJECT_0;
    }

    bool Semaphore::TryWait()
    {
        ASSERT(handle);
        u32 status = ::WaitForSingleObject(handle, 0);
        ASSERT(
            (status == WAIT_OBJECT_0) ||
            (status == WAIT_TIMEOUT));
        return status == WAIT_OBJECT_0;
    }

    SASSERT(sizeof(Mutex) >= sizeof(CRITICAL_SECTION));
    SASSERT(alignof(Mutex) >= alignof(CRITICAL_SECTION));

    static CRITICAL_SECTION* const AsCrit(Mutex* const pMutex)
    {
        return reinterpret_cast<CRITICAL_SECTION* const>(pMutex);
    }

    static const CRITICAL_SECTION* const AsCrit(const Mutex* const pMutex)
    {
        return reinterpret_cast<const CRITICAL_SECTION* const>(pMutex);
    }

    bool Mutex::IsOpen() const
    {
        HANDLE hdl = AsCrit(this)->LockSemaphore;
        return (created) && (hdl != INVALID_HANDLE_VALUE);
    }

    bool Mutex::Open()
    {
        created = ::InitializeCriticalSectionAndSpinCount(AsCrit(this), 1000);
        return created;
    }

    bool Mutex::Close()
    {
        if (IsOpen())
        {
            ::DeleteCriticalSection(AsCrit(this));
            created = false;
            return true;
        }
        return false;
    }

    void Mutex::Lock()
    {
        ASSERT(IsOpen());
        ::EnterCriticalSection(AsCrit(this));
    }

    void Mutex::Unlock()
    {
        ASSERT(IsOpen());
        ::LeaveCriticalSection(AsCrit(this));
    }

    bool Mutex::TryLock()
    {
        ASSERT(IsOpen());
        return ::TryEnterCriticalSection(AsCrit(this));
    }

    bool Event::Open()
    {
        Store(state, 0);
        return true;
    }

    bool Event::Close()
    {
        WakeAll();
        return true;
    }

    void Event::WakeOne()
    {
        Inc(state, MO_AcqRel);
        ::WakeByAddressSingle(&state);
    }

    void Event::WakeAll()
    {
        Inc(state, MO_AcqRel);
        ::WakeByAddressAll(&state);
    }

    void Event::Wake(i32 count)
    {
        for (i32 i = 0; i < count; ++i)
        {
            WakeOne();
        }
    }

    void Event::Wait()
    {
        i32 prev = Load(state);
        ::WaitOnAddress(&state, &prev, sizeof(state), INFINITE);
    }

};

#endif // PLAT_WINDOWS
