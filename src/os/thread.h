#pragma once

#include "common/int_types.h"

namespace OS
{
    using ThreadFn = ul32(*)(void* pData);

    struct Semaphore
    {
        void* handle;

        static Semaphore Create(u32 initialValue);
        inline bool IsOpen() const { return handle != 0; }
        void Close();
        void Signal(u32 count);
        void Wait();
    }; // Semaphore

    struct Thread
    {
        void* handle;
        u32 id;

        static Thread Self();
        static Thread Open(ThreadFn fn, void* data);
        inline bool IsOpen() const { return handle != 0; }
        void Join();

        static void Sleep(u32 ms);
        static void DoYield();
        static void Exit(u8 exitCode);
    }; // Thread

    struct Mutex
    {
        Semaphore sema;

        static Mutex Create() { return { Semaphore::Create(1) }; }
        inline bool IsOpen() const { return sema.IsOpen(); }
        inline void Close() { sema.Close(); };
        inline void Lock() { sema.Wait(); }
        inline void Unlock() { sema.Signal(1); }
    }; // Mutex

    struct LockGuard
    {
        Mutex& m_mtx;
        LockGuard(Mutex& mtx) : m_mtx(mtx) { m_mtx.Lock(); }
        ~LockGuard() { m_mtx.Unlock(); }
    }; // LockGuard

    struct Barrier
    {
        Mutex m_mutex;
        Semaphore m_sema;
        u32 m_size;
        u32 m_count;

        static Barrier Create(u32 size)
        {
            return {
                Mutex::Create(),
                Semaphore::Create(0),
                size,
                0
            };
        }
        inline void Wait()
        {
            m_mutex.Lock();
            if (++m_count == m_size)
            {
                m_count = 0;
                m_sema.Signal(m_size);
            }
            m_mutex.Unlock();
            m_sema.Wait();
        }
        inline void Close()
        {
            m_mutex.Close();
            m_sema.Close();
        }
        inline bool IsOpen() const
        {
            return m_mutex.IsOpen() && m_sema.IsOpen();
        }
    }; // Barrier

}; // OS
