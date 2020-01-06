#include "common/macro.h"

#if PLAT_WINDOWS

#include <windows.h>

#include "os/atomics.h"
#include "os/thread.h"
#include <intrin.h>

void OS::YieldProcessor()
{
    ::YieldProcessor();
}

u64 OS::ReadTimeStampCounter()
{
    return ::ReadTimeStampCounter();
}

void OS::SpinWait(u64 ticks)
{
    const u64 end = ::ReadTimeStampCounter() + ticks;
    do
    {
        YieldProcessor();
    } while (ReadTimeStampCounter() < end);
}

// ------------------------------------------------------------------------

struct ThreadAdapter
{
    OS::ThreadFn fn;
    void* data;
};

static constexpr isize MaxThreads = 64;
static ThreadAdapter ms_adapters[MaxThreads];
static Ai32 ms_adaptersInUse[MaxThreads];

static isize AllocAdapter(ThreadAdapter adapter)
{
    u64 spins = 0;
    while (spins < 100)
    {
        for (isize i = 0; i < MaxThreads; ++i)
        {
            i32 state = 0;
            if (ms_adaptersInUse[i].CmpExStrong(state, 1, MO_Acquire))
            {
                ASSERT(!state);
                ms_adapters[i].fn = adapter.fn;
                ms_adapters[i].data = adapter.data;
                return i;
            }
        }
        ++spins;
        OS::SpinWait(spins * 100);
    }
    ASSERT(false);
    return -1;
}

static void FreeAdapter(isize i)
{
    i32 state = ms_adaptersInUse[i].Dec(MO_Release);
    ASSERT(state == 1);
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
};

#endif // PLAT_WINDOWS
