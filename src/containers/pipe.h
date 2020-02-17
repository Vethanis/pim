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
    u8 pad[64 - sizeof(u32)];

    bool TryLockWriter()
    {
        u32 prev = kFlagWritable;
        return CmpExStrong(value, prev, kFlagLocked, MO_Acquire);
    }
    void UnlockWriter()
    {
        Store(value, kFlagReadable);
    }

    bool TryLockReader()
    {
        u32 prev = kFlagReadable;
        return CmpExStrong(value, prev, kFlagLocked, MO_Acquire);
    }
    void UnlockReader()
    {
        Store(value, kFlagWritable);
    }
};
SASSERT(sizeof(PipeFlag) == 64);

template<typename T, u32 kCapacity>
struct Pipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & kMask) == 0u);

    PipeFlag m_flags[kCapacity];
    u32 m_iWrite;
    T m_data[kCapacity];
    u32 m_iRead;

    u32 size() const { return Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    void Init() { Clear(); }
    void Clear() { memset(this, 0, sizeof(*this)); }

    bool TryPush(const T& src)
    {
        for (u32 i = Head(); size() <= kMask; i = Next(i))
        {
            if (m_flags[i].TryLockWriter())
            {
                m_data[i] = src;
                m_flags[i].UnlockWriter();
                Inc(m_iWrite, MO_Relaxed);
                return true;
            }
        }
        return false;
    }

    bool TryPop(T& dst)
    {
        for (u32 i = Tail(); size() != 0u; i = Next(i))
        {
            if (m_flags[i].TryLockReader())
            {
                dst = m_data[i];
                m_flags[i].UnlockReader();
                Inc(m_iRead, MO_Relaxed);
                return true;
            }
        }
        return false;
    }
};

