#pragma once

#include "common/macro.h"
#include "common/int_types.h"

namespace OS
{
    void YieldCore();
    u64 ReadCounter();
    void Spin(u64 ticks);
    void Rest(u64 ms);
    bool SwitchThread();

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
        mutable u32 m_state;
        mutable LightSema m_read;
        mutable LightSema m_write;

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

        void LockReader() const;
        void UnlockReader() const;
        void LockWriter();
        void UnlockWriter();
    };

    static RWLock CreateRWLock()
    {
        RWLock lock = {};
        lock.Open();
        return lock;
    }

    struct ReadGuard
    {
        const RWLock& m_lock;
        ReadGuard(const RWLock& lock) : m_lock(lock)
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
        mutable i32 m_state;

        bool TryLockReader() const;
        void LockReader() const;
        void UnlockReader() const;

        bool TryLockWriter();
        void LockWriter();
        void UnlockWriter();
    };

    struct ReadFlagGuard
    {
        const RWFlag& m_flag;
        ReadFlagGuard(const RWFlag& flag) : m_flag(flag)
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
