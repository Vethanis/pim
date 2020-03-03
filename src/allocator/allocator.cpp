#include "allocator/allocator.h"

#include "allocator/linear_allocator.h"
#include "allocator/stdlib_allocator.h"
#include "allocator/pool_allocator.h"

#include "os/thread.h"
#include "common/time.h"
#include "threading/task.h"

namespace Allocator
{
    static constexpr i32 kKilobyte = 1 << 10;
    static constexpr i32 kMegabyte = 1 << 20;
    static constexpr i32 kGigabyte = 1 << 30;

    static constexpr u32 kTempFrames = 4;
    static constexpr u32 kFrameMask = kTempFrames - 1u;

    static constexpr i32 kPermCapacity = 128 * kMegabyte;
    static constexpr i32 kTempCapacity = 4 * kMegabyte;
    static constexpr i32 kTaskCapacity = 1 * kMegabyte;

    static StdlibAllocator ms_initAllocator;
    static LinearAllocator ms_tempAllocators[kTempFrames];
    static PoolAllocator ms_taskAllocators[kNumThreads];
    static PoolAllocator ms_permAllocator;
    static OS::Mutex ms_permLock;

    static u32 GetFrame() { return Time::FrameCount() & kFrameMask; }

    void Init()
    {
        ms_initAllocator.Init(0, Alloc_Init);
        ms_permLock.Open();
        ms_permAllocator.Init(kPermCapacity, Alloc_Perm);
        for (auto& allocator : ms_tempAllocators)
        {
            allocator.Init(kTempCapacity, Alloc_Temp);
        }
        for (auto& allocator : ms_taskAllocators)
        {
            allocator.Init(kTaskCapacity, Alloc_Task);
        }
    }

    void Update()
    {
        ms_tempAllocators[GetFrame()].Clear();
    }

    void Shutdown()
    {
        ms_permLock.Lock();
        ms_initAllocator.Reset();
        for (auto& allocator : ms_tempAllocators)
        {
            allocator.Reset();
        }
        ms_permAllocator.Reset();
        for (auto& allocator : ms_taskAllocators)
        {
            allocator.Reset();
        }
        ms_permLock.Close();
    }

    // ------------------------------------------------------------------------

    void* Alloc(AllocType type, i32 bytes)
    {
        const u32 tid = ThreadId();
        const u32 frame = GetFrame();

        ASSERT(bytes >= 0);
        if (bytes <= 0)
        {
            return 0;
        }

        void* ptr = 0;

        switch (type)
        {
        case Alloc_Init:
            ptr = ms_initAllocator.Alloc(bytes);
            break;
        case Alloc_Temp:
            ptr = ms_tempAllocators[frame].Alloc(bytes);
            break;
        case Alloc_Perm:
            ms_permLock.Lock();
            ptr = ms_permAllocator.Alloc(bytes);
            ms_permLock.Unlock();
            break;
        case Alloc_Task:
            ptr = ms_taskAllocators[tid].Alloc(bytes);
            break;
        }

        ASSERT(ptr);
        ASSERT(((isize)ptr & 15) == 0);

        return ptr;
    }

    void* Realloc(AllocType type, void* pOldUser, i32 bytes)
    {
        const u32 tid = ThreadId();
        const u32 frame = GetFrame();

        ASSERT(bytes >= 0);

        if (!pOldUser)
        {
            return Alloc(type, bytes);
        }

        if (bytes <= 0)
        {
            Free(pOldUser);
            return 0;
        }

        void* ptr = 0;

        switch (UserToHeader(pOldUser, type)->type)
        {
        case Alloc_Init:
            ptr = ms_initAllocator.Realloc(pOldUser, bytes);
            break;
        case Alloc_Temp:
            ptr = ms_tempAllocators[frame].Realloc(pOldUser, bytes);
            break;
        case Alloc_Perm:
            ms_permLock.Lock();
            ptr = ms_permAllocator.Realloc(pOldUser, bytes);
            ms_permLock.Unlock();
            break;
        case Alloc_Task:
            ptr = ms_taskAllocators[tid].Realloc(pOldUser, bytes);
            break;
        }

        ASSERT(ptr);
        ASSERT(((isize)ptr & 15) == 0);

        return ptr;
    }

    void Free(void* ptr)
    {
        const u32 tid = ThreadId();
        const u32 frame = GetFrame();

        if (ptr)
        {
            ASSERT(((isize)ptr & 15) == 0);
            Header* hdr = (Header*)ptr;
            hdr -= 1;

            switch (hdr->type)
            {
            case Alloc_Init:
                ms_initAllocator.Free(ptr);
                break;
            case Alloc_Temp:
                ms_tempAllocators[frame].Free(ptr);
                break;
            case Alloc_Perm:
                ms_permLock.Lock();
                ms_permAllocator.Free(ptr);
                ms_permLock.Unlock();
                break;
            case Alloc_Task:
                ms_taskAllocators[tid].Free(ptr);
                break;
            }
        }
    }

    void* Calloc(AllocType type, i32 bytes)
    {
        void* ptr = Alloc(type, bytes);
        ASSERT(ptr);
        memset(ptr, 0, bytes);
        return ptr;
    }
};
