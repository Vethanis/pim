#pragma once

#include "containers/queue.h"
#include "os/thread.h"

template<typename T>
struct MtQueue
{
    mutable OS::RWLock m_lock;
    Queue<T> m_queue;

    void Init(AllocType allocator)
    {
        m_lock.Open();
        m_queue.Init(allocator);
    }

    void Reset()
    {
        m_lock.LockWriter();
        m_queue.Reset();
        m_lock.Close();
    }

    u32 size() const
    {
        OS::ReadGuard guard(m_lock);
        return m_queue.size();
    }

    u32 capacity() const
    {
        OS::ReadGuard guard(m_lock);
        return m_queue.capacity();
    }

    bool HasItems() const
    {
        return size() > 0u;
    }

    bool IsEmpty() const
    {
        return size() == 0u;
    }

    void Clear()
    {
        if (!IsEmpty())
        {
            OS::WriteGuard guard(m_lock);
            m_queue.Clear();
        }
    }

    void Trim()
    {
        m_lock.LockReader();
        const u32 sz = m_queue.size();
        const u32 cap = m_queue.capacity();
        m_lock.UnlockReader();

        if (HashUtil::ToPow2(sz) < cap)
        {
            OS::WriteGuard guard(m_lock);
            m_queue.Trim();
        }
    }

    void Reserve(u32 minCap)
    {
        if (minCap > capacity())
        {
            OS::WriteGuard guard(m_lock);
            m_queue.Reserve(minCap);
        }
    }

    void Push(const T& value)
    {
        OS::WriteGuard guard(m_lock);
        m_queue.Push(value);
    }

    void Push(const T& value, const Comparable<T>& cmp)
    {
        OS::WriteGuard guard(m_lock);
        m_queue.Push(value, cmp);
    }

    bool TryPop(T& valueOut)
    {
        if (size() > 0u)
        {
            OS::WriteGuard guard(m_lock);
            return m_queue.TryPop(valueOut);
        }
        return false;
    }
};
