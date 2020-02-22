#pragma once

#include "common/macro.h"
#include "os/atomics.h"
#include "allocator/allocator.h"

struct PipeFlag
{
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;
    static constexpr u32 kFlagLocked = 0xffffffffu;

    u32 value;

    bool IsWritable()
    {
        return Load(value) == kFlagWritable;
    }
    void SetReadable()
    {
        Store(value, kFlagReadable);
    }

    bool TryLockReader()
    {
        u32 prev = kFlagReadable;
        return CmpExStrong(value, prev, kFlagLocked);
    }
    void UnlockReader()
    {
        Store(value, kFlagWritable);
    }
};

template<typename T, u32 kCapacity>
struct Pipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & kMask) == 0u);

    u32 m_iWrite;
    u32 m_iRead;
    PipeFlag m_flags[kCapacity];
    T m_data[kCapacity];

    u32 size() const { return Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    void Init() { Clear(); }
    void Clear() { memset(this, 0, sizeof(*this)); }

    bool TryPush(const T& src)
    {
        u32 i = m_iWrite & kMask;
        if (m_flags[i].IsWritable())
        {
            m_data[i] = src;
            m_flags[i].SetReadable();
            Inc(m_iWrite);
            return true;
        }
        return false;
    }

    bool TryPop(T& dst)
    {
        for (u32 i = Tail(); size() != 0u; i = Next(i))
        {
            if (m_flags[i].TryLockReader())
            {
                Inc(m_iRead);
                dst = m_data[i];
                m_flags[i].UnlockReader();
                return true;
            }
        }
        return false;
    }
};

