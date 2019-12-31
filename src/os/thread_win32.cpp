
#include "common/macro.h"

#if PLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "common/macro.h"
#include "os/thread.h"
#include <stdlib.h>

namespace OS
{
    struct ThreadAdapter
    {
        ThreadFn fn;
        void* data;
    };

    static unsigned long Win32ThreadFn(void* pVoid)
    {
        ASSERT(pVoid);

        ThreadAdapter adapter = *(ThreadAdapter*)pVoid;
        free(pVoid);

        adapter.fn(adapter.data);

        return 0;
    }

    void Thread::Open(ThreadFn fn, void* data)
    {
        ASSERT(fn);

        ThreadAdapter* pAdapter = (ThreadAdapter*)malloc(sizeof(ThreadAdapter));
        ASSERT(pAdapter);

        pAdapter->fn = fn;
        pAdapter->data = data;

        ASSERT(!handle);
        // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread
        handle = ::CreateThread(
            nullptr,
            0,
            Win32ThreadFn,
            pAdapter,
            0,
            nullptr);
        ASSERT(handle);
    }

    void Thread::Join()
    {
        if (handle)
        {
            // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
            u32 status = ::WaitForSingleObject(handle, INFINITE);
            ASSERT(!status);

            // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
            bool closed = ::CloseHandle(handle);
            ASSERT(closed);
        }
        handle = nullptr;
    }

    // ----------------------------------------------------------------------------

    void Semaphore::Open(u32 initialValue)
    {
        constexpr u32 MaxValue = 0x7FFFFFFF;
        ASSERT(initialValue <= MaxValue);
        ASSERT(!handle);
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsemaphorea
        handle = ::CreateSemaphoreA(nullptr, initialValue, MaxValue, nullptr);
        ASSERT(handle);
    }

    void Semaphore::Close()
    {
        if (handle)
        {
            // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
            bool closed = ::CloseHandle(handle);
            ASSERT(closed);
        }
        handle = nullptr;
    }

    void Semaphore::Signal(u32 count)
    {
        ASSERT(handle);
        // increment by count
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
        bool rval = ::ReleaseSemaphore(handle, count, nullptr);
        ASSERT(rval);
    }

    void Semaphore::Wait()
    {
        ASSERT(handle);
        // decrement by one
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
        u32 status = ::WaitForSingleObject(handle, INFINITE);
        ASSERT(!status);
    }
};

#endif // PLAT_WINDOWS
