
#include "common/platform.h"

#if PLAT_WINDOWS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "os/thread.h"
#include "common/macro.h"

// ----------------------------------------------------------------------------

void Handle::Close()
{
    if (value)
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
        bool rval = ::CloseHandle(value);
        DebugAssert(rval);
        value = 0;
    }
}

// ----------------------------------------------------------------------------

Semaphore Semaphore::Create(u32 initialValue)
{
    constexpr u32 MaxValue = 0xFFFFFFF;
    DebugAssert(initialValue <= MaxValue);
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsemaphorea
    void* h = ::CreateSemaphoreA(0, initialValue, MaxValue, 0);
    DebugAssert(h);
    Semaphore s = {};
    s.handle.value = h;
    return s;
}

void Semaphore::Signal(u32 count)
{
    DebugAssert(handle.value);
    // increment by count
    // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-releasesemaphore
    bool rval = ::ReleaseSemaphore(handle.value, count, 0);
    DebugAssert(rval);
}

void Semaphore::Wait()
{
    DebugAssert(handle.value);
    // decrement by one
    // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
    u32 status = ::WaitForSingleObject(handle.value, INFINITE);
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
    Thread t = {};
    t.handle.value = hdl;
    t.id = id;
    return t;
}

void Thread::Join()
{
    if (handle.IsOpen())
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitforsingleobject
        u32 status = ::WaitForSingleObject(handle.value, INFINITE);
        DebugAssert(!status);
        handle.Close();
        id = 0;
    }
}

void Thread::Sleep(u32 ms)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep
    ::Sleep((DWORD)ms);
}

void Thread::Yield()
{
    // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread
    ::SwitchToThread();
}

void Thread::Exit(u8 exitCode)
{
    // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitthread
    ::ExitThread((DWORD)exitCode);
}

// ----------------------------------------------------------------------------

// https://docs.microsoft.com/en-us/windows/win32/ProcThread/creating-a-child-process-with-redirected-input-and-output
Process Process::Open(char* cmdline, cstr cwd)
{
    Process result = {};
    bool rval = true;

    void* hstdinRd = 0;
    void* hstdoutWr = 0;
    {
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        // https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-createpipe
        rval = ::CreatePipe(
            &hstdinRd,
            &result.hstdinWr.value,
            &sa,
            0);
        DebugAssert(rval);

        // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-sethandleinformation
        rval = ::SetHandleInformation(
            result.hstdinWr.value,
            HANDLE_FLAG_INHERIT,
            0);
        DebugAssert(rval);

        // https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-createpipe
        rval = ::CreatePipe(
            &result.hstdoutRd.value,
            &hstdoutWr,
            &sa,
            0);
        DebugAssert(rval);

        // https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-sethandleinformation
        rval = ::SetHandleInformation(
            result.hstdoutRd.value,
            HANDLE_FLAG_INHERIT,
            0);
        DebugAssert(rval);
    }

    STARTUPINFO startinfo = {};
    {
        startinfo.cb = sizeof(startinfo);
        startinfo.dwFlags |= STARTF_USESTDHANDLES;
        startinfo.hStdInput = hstdinRd;
        startinfo.hStdOutput = hstdoutWr;
    }

    PROCESS_INFORMATION proc = {};
    // https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
    rval = ::CreateProcess(
        0,          // const char* application name
        cmdline,    // char* command line
        0,          // LPSECURITY_ATTRIBUTES process attributes
        0,          // LPSECURITY_ATTRIBUTES thread attributes
        TRUE,       // BOOL inherit handles
        0,          // DWORD creation flags
        0,          // void* lpEnvironment
        cwd,        // lpCurrentDirectory
        &startinfo, // LPSTARTUPINFOA
        &proc);
    DebugAssert(rval);

    result.handle = { proc.hProcess };
    result.id = proc.dwProcessId;
    result.t0.handle = { proc.hThread };
    result.t0.id = proc.dwThreadId;

    return result;
}

bool Process::Write(const void* src, u32 len, u32& wrote)
{
    wrote = 0;
    DebugAssert(src);
    DebugAssert(handle.IsOpen());
    DebugAssert(hstdinWr.IsOpen());

    DWORD dwWrote = 0;
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
    bool rval = ::WriteFile(hstdinWr.value, src, (DWORD)len, &dwWrote, 0);
    wrote = dwWrote;

    return rval;
}

bool Process::Read(void* dst, u32 len, u32& read)
{
    read = 0;
    DebugAssert(dst);
    DebugAssert(handle.IsOpen());
    DebugAssert(hstdoutRd.IsOpen());

    DWORD dwRead = 0;
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
    bool rval = ::ReadFile(hstdoutRd.value, dst, (DWORD)len, &dwRead, 0);
    read = dwRead;

    return rval;
}

void Process::Join()
{
    t0.Join();
    handle.Close();
    id = 0;
    hstdinWr.Close();
    hstdoutRd.Close();
}

// ----------------------------------------------------------------------------

#endif // PLAT_WINDOWS
