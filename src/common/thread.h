#pragma once

#include "common/int_types.h"

#ifdef Yield
    #undef Yield
#endif // Yield

using ThreadFn = u32(*)(void* pData);

struct Handle
{
    void* value;

    inline bool IsOpen() const { return value != 0; }
    void Close();
};

struct Semaphore
{
    Handle handle;

    static Semaphore Create(u32 initialValue);
    inline bool IsOpen() const { return handle.IsOpen(); }
    inline void Close() { handle.Close(); };
    void Signal(u32 count);
    void Wait(u32 count);
};

struct Thread
{
    Handle handle;
    u32 id;

    static Thread Self();
    static Thread Open(ThreadFn fn, void* data);
    inline bool IsOpen() const { return handle.IsOpen(); }
    void Join();

    static void Sleep(u32 ms);
    static void Yield();
    static void Exit(u8 exitCode);
};

struct Process
{
    Handle handle;
    u32 id;
    Thread t0;
    Handle hstdinWr;
    Handle hstdoutRd;

    // cmdLine:
    //   batch files: cmd.exe /c <path to batch file> 
    // cwd: null for parent current directory
    static Process Open(char* cmdLine, cstr cwd);
    inline bool IsOpen() const { return handle.IsOpen(); }
    void Join();
    bool Write(const void* src, u32 len, u32& wrote);
    bool Read(void* dst, u32 len, u32& read);
};
