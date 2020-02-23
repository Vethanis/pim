#include "allocator/allocator.h"

#include "allocator/linear_allocator.h"
#include "allocator/stdlib_allocator.h"
#include "allocator/tlsf_allocator.h"

#include "containers/array.h"
#include "common/time.h"
#include "threading/task.h"

namespace Allocator
{
    static constexpr i32 kKilobyte = 1 << 10;
    static constexpr i32 kMegabyte = 1 << 20;
    static constexpr i32 kGigabyte = 1 << 30;

    static constexpr u32 kTempFrames = 4;
    static constexpr u32 kFrameMask = kTempFrames - 1u;

    static constexpr i32 kPermCapacity = 512 * kMegabyte;
    static constexpr i32 kTempCapacity = 1 * kMegabyte;
    static constexpr i32 kTaskCapacity = 1 * kMegabyte;

    static StdlibAllocator ms_initAllocator;
    static LinearAllocator ms_tempAllocators[kTempFrames];
    static TlsfAllocator ms_permAllocator;
    static TlsfAllocator ms_taskAllocators[kNumThreads];

    static u32 GetFrame() { return Time::FrameCount() & kFrameMask; }

    static IAllocator& GetAllocator(i32 type)
    {
        switch (type)
        {
        case Alloc_Init:
            return ms_initAllocator;
        case Alloc_Temp:
            return ms_tempAllocators[GetFrame()];
        case Alloc_Perm:
            return ms_permAllocator;
        case Alloc_Task:
            return ms_taskAllocators[ThreadId()];
        default:
            ASSERT(false);
            return ms_initAllocator;
        }
    }

    void Init()
    {
        ms_initAllocator.Init(0, Alloc_Init);
        for (auto& allocator : ms_tempAllocators)
        {
            allocator.Init(kTempCapacity, Alloc_Temp);
        }
        ms_permAllocator.Init(kPermCapacity, Alloc_Perm);
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
    }

    // ------------------------------------------------------------------------

    void* Alloc(AllocType type, i32 bytes)
    {
        ASSERT(bytes >= 0);
        void* ptr = GetAllocator(type).Alloc(bytes);
        ASSERT(ptr);
        return ptr;
    }

    void* Realloc(AllocType type, void* prev, i32 bytes)
    {
        ASSERT(bytes >= 0);
        if (!prev)
        {
            return Alloc(type, bytes);
        }
        if (bytes <= 0)
        {
            Free(prev);
            return 0;
        }
        Header* hdr = ToHeader(prev, type);
        void* ptr = GetAllocator(hdr->type).Realloc(prev, bytes);
        ASSERT(ptr);
        return ptr;
    }

    void Free(void* ptr)
    {
        if (ptr)
        {
            Header* hdr = (Header*)ptr;
            hdr -= 1;
            GetAllocator(hdr->type).Free(ptr);
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
