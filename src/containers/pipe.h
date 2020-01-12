#pragma once

#include "common/macro.h"
#include "containers/queue.h"
#include "os/atomics.h"

// multiple consumer, single producer pipe
template<typename T, u32 kCapacity>
struct ScatterPipe
{
    SASSERT((kCapacity & (kCapacity - 1u)) == 0u);

    static constexpr u32 kMask = kCapacity - 1u;
    static constexpr u32 kFlagInvalid = 0xffffffffu;
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;

    T m_data[kCapacity];

    u32 m_iWrite;
    u32 m_readCount;
    u32 m_flags[kCapacity];
    u32 m_iRead;

    void Clear()
    {
        Store(m_iWrite, 0u, MO_Relaxed);
        Store(m_iRead, 0u, MO_Relaxed);
        Store(m_readCount, 0u, MO_Relaxed);
        u32* const flags = m_flags;
        for (u32 i = 0; i < kCapacity; ++i)
        {
            Store(flags[i], 0u, MO_Relaxed);
        }
    }

    void Init()
    {
        Clear();
    }

    bool ReaderTryReadBack(T& dst)
    {
        u32 i = 0;
        u32 readCount = Load(m_readCount, MO_Relaxed);

        u32 readIndex = readCount;
        while (true)
        {
            u32 writeIndex = Load(m_iWrite, MO_Relaxed);
            u32 count = writeIndex - readCount;
            if (!count)
            {
                return false;
            }
            if (readIndex >= writeIndex)
            {
                readIndex = Load(m_iRead, MO_Relaxed);
            }

            i = readIndex & kMask;

            u32 prev = kFlagReadable;
            if (CmpExStrong(m_flags[i], prev, kFlagInvalid, MO_Acquire, MO_Relaxed))
            {
                break;
            }
            ++readIndex;
            readCount = Load(m_readCount, MO_Relaxed);
        }

        Inc(m_readCount, MO_Acquire);

        ThreadFenceAcquire();
        dst = m_data[i];
        Store(m_flags[i], kFlagWritable, MO_Release);

        return true;
    }

    bool WriterTryReadFront(T& dst)
    {
        u32 writeIndex = Load(m_iWrite, MO_Relaxed);
        u32 frontReadIndex = writeIndex;

        u32 i = 0;
        while (true)
        {
            u32 readCount = Load(m_readCount, MO_Relaxed);
            u32 count = writeIndex - readCount;
            if (!count)
            {
                Store(m_iRead, readCount, MO_Release);
                return false;
            }

            --frontReadIndex;
            i = frontReadIndex & kMask;

            u32 prev = kFlagReadable;
            if (CmpExStrong(m_flags[i], prev, kFlagInvalid, MO_Acquire, MO_Relaxed))
            {
                break;
            }

            if (Load(m_iRead, MO_Acquire) >= frontReadIndex)
            {
                return false;
            }
        }

        ThreadFenceAcquire();

        dst = m_data[i];
        Store(m_flags[i], kFlagReadable, MO_Release);
        Dec(m_iWrite, MO_Release);

        return true;
    }

    bool WriterTryWriteFront(const T& src)
    {
        u32 writeIndex = Load(m_iWrite, MO_Relaxed);
        u32 i = writeIndex & kMask;

        if (Load(m_flags[i], MO_Relaxed) != kFlagWritable)
        {
            return false;
        }

        m_data[i] = src;
        Store(m_flags[i], kFlagReadable, MO_Release);

        ThreadFenceRelease();

        Inc(m_iWrite, MO_Release);

        return true;
    }
};
