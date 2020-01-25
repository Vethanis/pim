#pragma once

#include "common/macro.h"
#include "os/atomics.h"

template<typename T, u32 kCapacity>
struct Pipe
{
    SASSERT((kCapacity & (kCapacity - 1u)) == 0u);

    static constexpr u32 kMask = kCapacity - 1u;
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;
    static constexpr u32 kFlagLocked = 0xffffffffu;
    static constexpr u32 kNotFound = kFlagLocked;

    u32 m_iWrite;
    u32 m_iRead;
    u32 m_flags[kCapacity];
    T m_data[kCapacity];

    void Init()
    {
        Clear();
    }

    void Clear()
    {
        Store(m_iWrite, 0u, MO_Release);
        Store(m_iRead, 0u, MO_Release);
        u32* const flags = m_flags;
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(flags[i], kFlagWritable, MO_Release);
        }
    }

    u32 size() const
    {
        return (Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed)) & kMask;
    }

    bool IsFull() const { return size() == kMask; }
    bool IsNotFull() const { return size() < kMask; }
    bool IsEmpty() const { return size() == 0u; }
    bool IsNotEmpty() const { return size() > 0u; }

    u32 LockSlot(u32 start, u32 searchFlag, u32 quitSize)
    {
        u32 i = start;
        while (size() != quitSize)
        {
            i &= kMask;
            u32 prev = searchFlag;
            if (CmpExStrong(m_flags[i], prev, kFlagLocked, MO_Acquire, MO_Relaxed))
            {
                return i;
            }
            ++i;
        }
        return kNotFound;
    }

    bool TryPush(const T& src)
    {
        const u32 i = LockSlot(Load(m_iWrite, MO_Relaxed), kFlagWritable, kMask);
        if (i != kNotFound)
        {
            m_data[i] = src;
            Store(m_flags[i], kFlagReadable, MO_Release);
            Inc(m_iWrite, MO_Release);
            return true;
        }
        return false;
    }

    bool TryPop(T& dst)
    {
        const u32 i = LockSlot(Load(m_iRead, MO_Relaxed), kFlagReadable, 0u);
        if (i != kNotFound)
        {
            dst = m_data[i];
            Store(m_flags[i], kFlagWritable, MO_Release);
            Inc(m_iRead, MO_Release);
            return true;
        }
        return false;
    }
};

template<u32 kCapacity>
struct PtrPipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & (kCapacity - 1u)) == 0u);

    u32 m_iWrite;
    u32 m_iRead;
    isize m_ptrs[kCapacity];

    void Init()
    {
        Clear();
    }

    void Clear()
    {
        Store(m_iWrite, 0u, MO_Release);
        Store(m_iRead, 0u, MO_Release);
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(m_ptrs[i], 0, MO_Release);
        }
    }

    u32 size() const
    {
        return (Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed)) & kMask;
    }

    bool IsFull() const { return size() == kMask; }
    bool IsNotFull() const { return size() < kMask; }
    bool IsEmpty() const { return size() == 0u; }
    bool IsNotEmpty() const { return size() > 0u; }

    bool TryPush(void* ptr)
    {
        const isize iPtr = (isize)ptr;
        u32 i = Load(m_iWrite, MO_Relaxed);
        isize* const ptrs = m_ptrs;
        while (IsNotFull())
        {
            i &= kMask;
            isize prevPtr = Load(ptrs[i], MO_Relaxed);
            if (!prevPtr)
            {
                if (CmpExStrong(ptrs[i], prevPtr, iPtr, MO_Acquire, MO_Relaxed))
                {
                    ASSERT(!prevPtr);
                    Inc(m_iWrite, MO_Release);
                    return true;
                }
            }
            ++i;
        }
        return false;
    }

    void* TryPop()
    {
        u32 i = Load(m_iRead, MO_Relaxed);
        isize* const ptrs = m_ptrs;
        while (IsNotEmpty())
        {
            i &= kMask;
            isize prevPtr = Load(ptrs[i], MO_Relaxed);
            if (prevPtr)
            {
                if (CmpExStrong(ptrs[i], prevPtr, 0, MO_Acquire, MO_Relaxed))
                {
                    ASSERT(prevPtr);
                    Inc(m_iRead, MO_Release);
                    return (void*)prevPtr;
                }
            }
            ++i;
        }
        return nullptr;
    }
};
