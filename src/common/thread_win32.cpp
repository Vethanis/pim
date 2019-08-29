
#include "common/platform.h"

#if PLAT_WINDOWS

#include <windows.h>

#include "common/thread.h"
#include "common/macro.h"

// ----------------------------------------------------------------------------

void Handle::Close()
{
    if (value)
    {
        ::CloseHandle(value);
        value = 0;
    }
}

// ----------------------------------------------------------------------------

Semaphore Semaphore::Create(u32 initialValue)
{
    void* h = ::CreateSemaphore(0, initialValue, 0xffff, 0);
    DebugAssert(h);
    return { h };
}

void Semaphore::Signal(u32 count)
{
    DebugAssert(handle.IsOpen());
    ::ReleaseSemaphore(handle.value, count, 0);
}

void Semaphore::Wait(u32 count)
{
    DebugAssert(handle.IsOpen());
    for (u32 i = 0; i < count; ++i)
    {
        ::WaitForSingleObject(handle.value, INFINITE);
    }
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
    return { { hdl }, id };
}

void Thread::Join()
{
    if (handle.IsOpen())
    {
        ::WaitForSingleObject(handle.value, INFINITE);
        handle.Close();
        id = 0;
    }
}

void Thread::Sleep(u32 ms)
{
    ::Sleep((DWORD)ms);
}

void Thread::Yield()
{
    ::SwitchToThread();
}

void Thread::Exit(u8 exitCode)
{
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

        rval = ::CreatePipe(
            &hstdinRd,
            &result.hstdinWr.value,
            &sa,
            0);
        DebugAssert(rval);

        rval = ::SetHandleInformation(
            result.hstdinWr.value,
            HANDLE_FLAG_INHERIT,
            0);
        DebugAssert(rval);

        rval = ::CreatePipe(
            &result.hstdoutRd.value,
            &hstdoutWr,
            &sa,
            0);
        DebugAssert(rval);

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
