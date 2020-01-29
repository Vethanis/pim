#pragma once

#include "common/int_types.h"

namespace OS
{
    void YieldCore();
    u64 ReadCounter();
    void Spin(u64 ticks);
    void Rest(u64 ms);

    using ThreadFn = void(*)(void* pData);

    struct Thread
    {
        void* handle;

        bool IsOpen() const { return handle != nullptr; }
        bool Open(ThreadFn fn, void* data);
        bool Join();
    };

    struct Semaphore
    {
        void* handle;

        bool IsOpen() const { return handle != nullptr; }
        bool Open(u32 initialValue = 0);
        bool Close();
        bool Signal(u32 count = 1);
        bool Wait();
        bool TryWait();
    };

    struct SpinLock
    {
        i32 m_count;

        bool TryLock();
        void Lock();
        void Unlock();
    };

    struct LightSema
    {
        Semaphore m_sema;
        i32 m_count;

        bool IsOpen() const { return m_sema.IsOpen(); }
        bool Open(u32 initialValue = 0)
        {
            m_count = (i32)initialValue;
            return m_sema.Open(initialValue);
        }
        bool Close() { return m_sema.Close(); }
        bool TryWait();
        void Wait();
        void Signal(i32 count = 1);
    };

    struct Mutex
    {
        LightSema sema;

        bool IsOpen() const { return sema.IsOpen(); }
        bool Open() { return sema.Open(1); }
        bool Close() { return sema.Close(); };
        void Lock() { sema.Wait(); }
        void Unlock() { sema.Signal(); }
        bool TryLock() { return sema.TryWait(); }
    };

    struct LockGuard
    {
        Mutex& m_mtx;
        LockGuard(Mutex& mtx) : m_mtx(mtx) { m_mtx.Lock(); }
        ~LockGuard() { m_mtx.Unlock(); }
    };

    struct RWLock
    {
        u32 m_state;
        LightSema m_read;
        LightSema m_write;

        void Open()
        {
            m_state = 0;
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

    struct ReadGuard
    {
        RWLock& m_lock;
        ReadGuard(RWLock& lock) : m_lock(lock)
        {
            lock.LockReader();
        }
        ~ReadGuard()
        {
            m_lock.UnlockReader();
        }
    };

    struct WriteGuard
    {
        RWLock& m_lock;
        WriteGuard(RWLock& lock) : m_lock(lock)
        {
            lock.LockWriter();
        }
        ~WriteGuard()
        {
            m_lock.UnlockWriter();
        }
    };

    struct Event
    {
        i32 m_waits;
        LightSema m_sema;

        bool Open()
        {
            m_waits = 0;
            return m_sema.Open();
        }
        bool Close()
        {
            return m_sema.Close();
        }

        void WakeOne();
        void WakeAll();
        void Wait();
    };

    struct RWFlag
    {
        i32 m_state;

        bool TryLockReader();
        void LockReader()
        {
            u64 spins = 0;
            while (!TryLockReader())
            {
                OS::Spin(++spins * 100);
            }
        }
        void UnlockReader();

        bool TryLockWriter();
        void LockWriter()
        {
            u64 spins = 0;
            while (!TryLockWriter())
            {
                OS::Spin(++spins * 100);
            }
        }
        void UnlockWriter();
    };

    struct ReadFlagGuard
    {
        RWFlag& m_flag;
        ReadFlagGuard(RWFlag& flag) : m_flag(flag)
        {
            m_flag.LockReader();
        }
        ~ReadFlagGuard()
        {
            m_flag.UnlockReader();
        }
    };

    struct WriteFlagGuard
    {
        RWFlag& m_flag;
        WriteFlagGuard(RWFlag& flag) : m_flag(flag)
        {
            m_flag.LockWriter();
        }
        ~WriteFlagGuard()
        {
            m_flag.UnlockWriter();
        }
    };
};
