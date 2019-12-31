#pragma once

#include "common/int_types.h"

namespace OS
{
    using ThreadFn = void(*)(void* pData);

    struct Thread
    {
        void* handle;

        inline bool IsOpen() const { return handle != nullptr; }
        void Open(ThreadFn fn, void* data);
        void Join();
    };

    struct Semaphore
    {
        void* handle;

        inline bool IsOpen() const { return handle != nullptr; }
        void Open(u32 initialValue);
        void Close();
        void Signal(u32 count);
        void Wait();
    };

    struct Mutex
    {
        Semaphore sema;

        inline bool IsOpen() const { return sema.IsOpen(); }
        inline void Open() { sema.Open(1); }
        inline void Close() { sema.Close(); };
        inline void Lock() { sema.Wait(); }
        inline void Unlock() { sema.Signal(1); }
    };

    struct LockGuard
    {
        Mutex& m_mtx;
        LockGuard(Mutex& mtx) : m_mtx(mtx) { m_mtx.Lock(); }
        ~LockGuard() { m_mtx.Unlock(); }
    };
};
