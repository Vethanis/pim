#pragma once

#include "containers/queue.h"
#include "os/thread.h"

template<typename T>
struct MtQueue
{
    static constexpr u32 kFlagWritable = 0x00000000u;
    static constexpr u32 kFlagReadable = 0x11111111u;
    static constexpr u32 kFlagLocked = 0xffffffffu;

    mutable OS::RWLock m_lock;
    u32 m_iWrite;
    u32 m_iRead;
    u32 m_width;
    T* m_ptr;
    u32* m_flags;
    AllocType m_allocator;

    void Init(AllocType allocator, u32 minCap = 0u)
    {
        memset(this, 0, sizeof(*this));
        m_lock.Open();
        m_allocator = allocator;
        if (minCap > 0u)
        {
            Reserve(minCap);
        }
    }

    void Reset()
    {
        m_lock.LockWriter();
        Store(m_width, 0);
        Store(m_iWrite, 0);
        Store(m_iRead, 0);
        T* ptr = ExchangePtr(m_ptr, (T*)nullptr);
        u32* flags = ExchangePtr(m_flags, (u32*)nullptr);
        Allocator::Free(ptr);
        Allocator::Free(flags);
        m_lock.Close();
    }

    u32 capacity() const { return Load(m_width); }
    u32 size() const { return Load(m_iWrite) - Load(m_iRead); }

    void Clear()
    {
        Store(m_iWrite, 0);
        Store(m_iRead, 0);
        OS::ReadGuard guard(m_lock);
        const u32 width = Load(m_width);
        u32* const flags = m_flags;
        for (u32 i = 0; i < width; ++i)
        {
            Store(flags[i], kFlagWritable);
        }
    }

    void Reserve(u32 minCap)
    {
        const u32 newWidth = ToPow2(minCap);
        if (newWidth > Load(m_width))
        {
            T* newPtr = Allocator::CallocT<T>(m_allocator, newWidth);
            u32* newFlags = Allocator::CallocT<u32>(m_allocator, newWidth);

            m_lock.LockWriter();

            T* oldPtr = LoadPtr(m_ptr);
            u32* oldFlags = LoadPtr(m_flags);
            const u32 oldWidth = Load(m_width);
            const bool grew = oldWidth < newWidth;

            if (grew)
            {
                const u32 oldMask = oldWidth - 1u;
                const u32 oldTail = Load(m_iRead);
                const u32 oldHead = Load(m_iWrite);
                const u32 len = oldHead - oldTail;

                for (u32 i = 0u; i < len; ++i)
                {
                    const u32 iOld = (oldTail + i) & oldMask;
                    newPtr[i] = oldPtr[iOld];
                    newFlags[i] = oldFlags[iOld];
                }

                StorePtr(m_ptr, newPtr);
                StorePtr(m_flags, newFlags);
                Store(m_width, newWidth);
                Store(m_iRead, 0);
                Store(m_iWrite, len);
            }

            m_lock.UnlockWriter();

            if (grew)
            {
                Allocator::Free(oldPtr);
                Allocator::Free(oldFlags);
            }
            else
            {
                Allocator::Free(newPtr);
                Allocator::Free(newFlags);
            }
        }
    }

    void Push(const T& value)
    {
        while (true)
        {
            Reserve(size() + 16u);
            OS::ReadGuard guard(m_lock);
            const u32 mask = Load(m_width) - 1u;
            T* const ptr = LoadPtr(m_ptr);
            u32* const flags = LoadPtr(m_flags);
            for (u32 i = Load(m_iWrite); size() < mask; ++i)
            {
                i &= mask;
                u32 prev = kFlagWritable;
                if (CmpExStrong(flags[i], prev, kFlagLocked, MO_Acquire))
                {
                    ptr[i] = value;
                    Store(flags[i], kFlagReadable);
                    Inc(m_iWrite);
                    return;
                }
            }
        }
    }

    bool TryPop(T& valueOut)
    {
        if (!size())
        {
            return false;
        }
        OS::ReadGuard guard(m_lock);
        const u32 mask = Load(m_width) - 1u;
        T* const ptr = LoadPtr(m_ptr);
        u32* const flags = LoadPtr(m_flags);
        for (u32 i = Load(m_iRead); size() != 0u; ++i)
        {
            i &= mask;
            u32 prev = kFlagReadable;
            if (CmpExStrong(flags[i], prev, kFlagLocked, MO_Acquire))
            {
                valueOut = ptr[i];
                Store(flags[i], kFlagWritable);
                Inc(m_iRead);
                return true;
            }
        }
        return false;
    }
};
