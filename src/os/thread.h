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

    struct alignas(8) Mutex
    {
        u64 opaque[5];
        bool created;

        bool IsOpen() const;
        bool Open();
        bool Close();
        void Lock();
        void Unlock();
        bool TryLock();
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
        mutable Semaphore m_read;
        mutable Semaphore m_write;

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
        i32 state;

        bool Open();
        bool Close();
        void WakeOne();
        void WakeAll();
        void Wake(i32 count);
        void Wait();
    };

    struct RWFlag
    {
        mutable i16 m_state;

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
