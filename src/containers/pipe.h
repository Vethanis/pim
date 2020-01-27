#pragma once

#include "common/macro.h"
#include "os/atomics.h"

template<typename T, u32 kCapacity>
struct Pipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;
    static constexpr u32 kFlagLocked = 0xffffffffu;
    SASSERT((kCapacity & kMask) == 0u);

    u32 m_iWrite;
    u32 m_iRead;
    u32 m_flags[kCapacity];
    T m_data[kCapacity];

    u32 size() const { return Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    void Init() { Clear(); }
    void Clear()
    {
        Store(m_iWrite, 0u);
        Store(m_iRead, 0u);
        u32* const flags = m_flags;
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(flags[i], kFlagWritable);
        }
    }

    bool TryPush(const T& src)
    {
        for (u32 i = Head(); size() < kMask; i = Next(i))
        {
            u32 prev = kFlagWritable;
            if (CmpExStrong(m_flags[i], prev, kFlagLocked))
            {
                m_data[i] = src;
                Store(m_flags[i], kFlagReadable);
                Inc(m_iWrite);
                return true;
            }
        }
        return false;
    }

    bool TryPop(T& dst)
    {
        for (u32 i = Tail(); size() > 0u; i = Next(i))
        {
            u32 prev = kFlagReadable;
            if (CmpExStrong(m_flags[i], prev, kFlagLocked))
            {
                dst = m_data[i];
                Store(m_flags[i], kFlagWritable);
                Inc(m_iRead);
                return true;
            }
        }
        return false;
    }
};

template<u32 kCapacity>
struct PtrPipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & kMask) == 0u);

    u32 m_iWrite;
    u32 m_iRead;
    isize m_ptrs[kCapacity];

    void Init() { Clear(); }
    void Clear()
    {
        Store(m_iWrite, 0u);
        Store(m_iRead, 0u);
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(m_ptrs[i], 0);
        }
    }

    u32 size() const { return Load(m_iWrite, MO_Relaxed) - Load(m_iRead, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    bool TryPush(void* ptr)
    {
        ASSERT(ptr);
        for (u32 i = Head(); size() < kMask; i = Next(i))
        {
            isize prev = 0;
            if (CmpExStrong(m_ptrs[i], prev, (isize)ptr))
            {
                Inc(m_iWrite);
                ASSERT(!prev);
                return true;
            }
        }
        return false;
    }

    void* TryPop()
    {
        for (u32 i = Tail(); size() > 0u; i = Next(i))
        {
            isize prev = Load(m_ptrs[i]);
            if (prev && CmpExStrong(m_ptrs[i], prev, 0))
            {
                Inc(m_iRead);
                ASSERT(prev);
                return (void*)prev;
            }
        }
        return nullptr;
    }
};
