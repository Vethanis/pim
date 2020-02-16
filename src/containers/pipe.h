#pragma once

#include "common/macro.h"
#include "os/atomics.h"
#include "allocator/allocator.h"

struct alignas(64) PipeFlag
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
        ASSERT(Load(value, MO_Relaxed) == kFlagLocked);
        Store(value, kFlagReadable, MO_Release);
    }
    bool TryLockReader()
    {
        u32 prev = kFlagReadable;
        return CmpExStrong(value, prev, kFlagLocked, MO_Acquire);
    }
    void UnlockReader()
    {
        ASSERT(Load(value, MO_Relaxed) == kFlagLocked);
        Store(value, kFlagWritable, MO_Release);
    }
};
SASSERT(sizeof(PipeFlag) == 64);

struct alignas(64) PtrLine
{
    isize m_ptr;
    u8 m_pad[64 - sizeof(isize)];

    bool TryWrite(isize valueIn)
    {
        isize prev = 0;
        return CmpExStrong(m_ptr, prev, valueIn, MO_Release);
    }

    bool TryRead(isize& valueOut)
    {
        isize ptr = Load(m_ptr, MO_Relaxed);
        if (ptr && CmpExStrong(m_ptr, ptr, 0, MO_Acquire))
        {
            valueOut = ptr;
            return true;
        }
        return false;
    }
};
SASSERT(sizeof(PtrLine) == 64);

struct alignas(64) u32Line
{
    u32 Value;
    u8 Pad[64 - sizeof(u32)];
};
SASSERT(sizeof(u32Line) == 64);

template<typename T, u32 kCapacity>
struct Pipe
{
    static constexpr u32 kMask = kCapacity - 1u;
    SASSERT((kCapacity & kMask) == 0u);

    u32Line m_iWrite;
    u32Line m_iRead;
    PipeFlag m_flags[kCapacity];
    T m_data[kCapacity];

    u32 size() const { return Load(m_iWrite.Value, MO_Relaxed) - Load(m_iRead.Value, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite.Value, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead.Value, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    void Init() { Clear(); }
    void Clear() { memset(this, 0, sizeof(*this)); }

    bool TryPush(const T& src)
    {
        for (u32 i = Head(); size() <= kMask; i = Next(i))
        {
            u32 prev = kFlagWritable;
            if (m_flags[i].TryLockWriter())
            {
                m_data[i] = src;
                m_flags[i].UnlockWriter();
                Inc(m_iWrite.Value, MO_Relaxed);
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
            if (m_flags[i].TryLockReader())
            {
                dst = m_data[i];
                m_flags[i].UnlockReader();
                Inc(m_iRead.Value, MO_Relaxed);
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

    u32Line m_iWrite;
    u32Line m_iRead;
    PtrLine m_ptrs[kCapacity];

    void Init() { Clear(); }
    void Clear() { memset(this, 0, sizeof(*this)); }

    u32 size() const { return Load(m_iWrite.Value, MO_Relaxed) - Load(m_iRead.Value, MO_Relaxed); }
    u32 Head() const { return Load(m_iWrite.Value, MO_Relaxed) & kMask; }
    u32 Tail() const { return Load(m_iRead.Value, MO_Relaxed) & kMask; }
    static u32 Next(u32 i) { return (i + 1u) & kMask; }

    bool TryPush(void* ptr)
    {
        ASSERT(ptr);
        PtrLine* const ptrs = m_ptrs;
        for (u32 i = Head(); size() <= kMask; i = Next(i))
        {
            isize prev = 0;
            if (ptrs[i].TryWrite((isize)ptr))
            {
                Inc(m_iWrite.Value, MO_Relaxed);
                ASSERT(!prev);
                return true;
            }
        }
        return false;
    }

    void* TryPop()
    {
        isize ptr = 0;
        PtrLine* const ptrs = m_ptrs;
        for (u32 i = Tail(); size() != 0u; i = Next(i))
        {
            if (ptrs[i].TryRead(ptr))
            {
                Inc(m_iRead.Value, MO_Relaxed);
                ASSERT(ptr);
                return (void*)ptr;
            }
        }
        return nullptr;
    }
};
