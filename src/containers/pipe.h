#pragma once

#include "common/macro.h"
#include "os/atomics.h"
#include "allocator/allocator.h"

template<typename T, u32 kCapacity>
struct Pipe
{
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;
    static constexpr u32 kFlagLocked = 0xffffffffu;
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & kMask) == 0u);

    u32 m_iWrite;
    u32 m_iRead;
    u32 m_flags[kCapacity];
    T m_data[kCapacity];

    u32 size() const { return Load(m_iWrite) - Load(m_iRead); }
    u32 Head() const { return Load(m_iWrite) & kMask; }
    u32 Tail() const { return Load(m_iRead) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    void Init() { Clear(); }
    void Clear() { memset(this, 0, sizeof(*this)); }

    bool TryPush(const T& src)
    {
        for (u32 i = Head(); size() <= kMask; i = Next(i))
        {
            u32 prev = kFlagWritable;
            if (CmpExStrong(m_flags[i], prev, kFlagLocked, MO_Acquire))
            {
                m_data[i] = src;
                Store(m_flags[i], kFlagReadable);
                Inc(m_iWrite, MO_Release);
                return true;
            }
        }
        return false;
    }

    bool TryPop(T& dst)
    {
        for (u32 i = Tail(); size() != 0u; i = Next(i))
        {
            u32 prev = kFlagReadable;
            if (CmpExStrong(m_flags[i], prev, kFlagLocked, MO_Acquire))
            {
                dst = m_data[i];
                Store(m_flags[i], kFlagWritable);
                Inc(m_iRead, MO_Release);
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
    void Clear() { memset(this, 0, sizeof(*this)); }

    u32 size() const { return Load(m_iWrite.Value) - Load(m_iRead.Value); }
    u32 Head() const { return Load(m_iWrite.Value) & kMask; }
    u32 Tail() const { return Load(m_iRead.Value) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    bool TryPush(void* ptr)
    {
        ASSERT(ptr);
        for (u32 i = Head(); size() <= kMask; i = Next(i))
        {
            isize prev = 0;
            if (CmpExStrong(m_ptrs[i], prev, (isize)ptr, MO_Acquire))
            {
                Inc(m_iWrite.Value, MO_Release);
                ASSERT(!prev);
                return true;
            }
        }
        return false;
    }

    void* TryPop()
    {
        for (u32 i = Tail(); size() != 0u; i = Next(i))
        {
            isize ptr = Load(m_ptrs[i], MO_Relaxed);
            if (ptr && CmpExStrong(m_ptrs[i], ptr, 0, MO_Acquire))
            {
                Inc(m_iRead.Value, MO_Release);
                ASSERT(ptr);
                return (void*)ptr;
            }
        }
        return nullptr;
    }
};
