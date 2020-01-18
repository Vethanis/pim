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
        Store(m_iWrite, 0u, MO_Relaxed);
        Store(m_iRead, 0u, MO_Relaxed);
        u32* const flags = m_flags;
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(flags[i], kFlagWritable, MO_Relaxed);
        }
    }

    u32 size() const
    {
        return (Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed)) & kMask;
    }

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
