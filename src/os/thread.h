#pragma once

#include "common/int_types.h"
#include "os/atomic_types.h"

namespace OS
{
    void YieldProcessor();
    u64 ReadTimeStampCounter();
    void SpinWait(u64 ticks);

    using ThreadFn = void(*)(void* pData);

    struct Thread
    {
        void* handle;

        inline bool IsOpen() const { return handle != nullptr; }
        bool Open(ThreadFn fn, void* data);
        bool Join();
    };

    struct Semaphore
    {
        void* handle;

        inline bool IsOpen() const { return handle != nullptr; }
        bool Open(u32 initialValue = 0);
        bool Close();
        bool Signal(u32 count = 1);
        bool Wait();
        bool TryWait();
    };

    struct SpinLock
    {
        a32 m_count;

        bool TryLock();
        void Lock();
        void Unlock();
    };

    struct LightSema
    {
        Semaphore m_sema;
        a32 m_count;

        inline bool IsOpen() const { return m_sema.IsOpen(); }
        inline bool Open(u32 initialValue = 0)
        {
            m_count.Value = (i32)initialValue;
            return m_sema.Open(initialValue);
        }
        inline bool Close() { return m_sema.Close(); }
        bool TryWait();
        void Wait();
        void Signal(i32 count = 1);
    };

    struct Mutex
    {
        LightSema sema;

        inline bool IsOpen() const { return sema.IsOpen(); }
        inline bool Open() { return sema.Open(1); }
        inline bool Close() { return sema.Close(); };
        inline void Lock() { sema.Wait(); }
        inline void Unlock() { sema.Signal(); }
        inline bool TryLock() { return sema.TryWait(); }
    };

    struct LockGuard
    {
        Mutex& m_mtx;
        LockGuard(Mutex& mtx) : m_mtx(mtx) { m_mtx.Lock(); }
        ~LockGuard() { m_mtx.Unlock(); }
    };

    struct RWLock
    {
        a32 m_state;
        LightSema m_read;
        LightSema m_write;

        void Open()
        {
            m_state.Value = 0;
            m_read.Open();
            m_write.Open();
        }
        void Close()
        {
            m_read.Close();
            m_write.Close();
        }

        void LockReader();
        void UnlockReader();
        void LockWriter();
        void UnlockWriter();
    };

    struct Event
    {
        a32 m_state;
        LightSema m_sema;

        bool Open(bool signalled = false)
        {
            m_state.Value = signalled ? 1 : 0;
            return m_sema.Open(signalled ? 1u : 0u);
        }
        bool Close()
        {
            return m_sema.Close();
        }

        void Signal();
        void Wait();
    };
};
