
#include "common/platform.h"

#if PLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "os/thread.h"
#include "common/macro.h"

namespace OS
{
    Semaphore Semaphore::Create(u32 initialValue)
    {
        constexpr u32 MaxValue = 0xFFFFFFF;
        DebugAssert(initialValue <= MaxValue);
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsemaphorea
        void* h = ::CreateSemaphoreA(0, initialValue, MaxValue, 0);
        DebugAssert(h);
        return { h };
    }

    void Semaphore::Close()
    {
        if (handle)
        {
            bool closed = ::CloseHandle(handle);
            DebugAssert(closed);
            handle = 0;
        }
    }

    void Semaphore::Signal(u32 count)
    {
        DebugAssert(handle);
        // increment by count
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
        bool rval = ::ReleaseSemaphore(handle, count, 0);
        DebugAssert(rval);
    }

    void Semaphore::Wait()
    {
        DebugAssert(handle);
        // decrement by one
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
        u32 status = ::WaitForSingleObject(handle, INFINITE);
        DebugAssert(!status);
    }

    // ----------------------------------------------------------------------------

    Thread Thread::Self()
    {
        return { ::GetCurrentThread(), ::GetCurrentThreadId() };
    }

    Thread Thread::Open(ThreadFn fn, void* data)
    {
        DWORD id = 0;
        HANDLE hdl = ::CreateThread(
            0,
            0,
            (LPTHREAD_START_ROUTINE)fn,
            data,
            0,
            &id);
        DebugAssert(hdl);
        return { hdl, id };
    }

    void Thread::Join()
    {
        if (handle)
        {
            // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
            u32 status = ::WaitForSingleObject(handle, INFINITE);
            DebugAssert(!status);
            bool closed = ::CloseHandle(handle);
            DebugAssert(closed);
            handle = 0;
            id = 0;
        }
    }

    void Thread::Sleep(u32 ms)
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep
        ::Sleep((DWORD)ms);
    }

    void Thread::DoYield()
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread
        ::SwitchToThread();
    }

    void Thread::Exit(u8 exitCode)
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitthread
        ::ExitThread((DWORD)exitCode);
    }

}; // OS

#endif // PLAT_WINDOWS
