#include "allocator/allocator.h"

#include "allocator/linear_allocator.h"
#include "allocator/pool_allocator.h"
#include "allocator/stack_allocator.h"
#include "allocator/stdlib_allocator.h"

#include "containers/array.h"
#include "common/hashstring.h"
#include "common/round.h"
#include <stdlib.h>

#if ENABLE_LEAK_TRACKER
#include "allocator/leak_tracker.h"
#endif

namespace Allocator
{
    static StdlibAllocator ms_stdlib = StdlibAllocator(Alloc_Stdlib);
    static StdlibAllocator ms_debug = StdlibAllocator(Alloc_Debug);
    static LinearAllocator ms_linear = LinearAllocator(1 << 20);
    static StackAllocator ms_stack = StackAllocator(1 << 20);
    static PoolAllocator ms_pool = PoolAllocator(64 << 20);

#if ENABLE_LEAK_TRACKER
    static LeakTracker ms_tracker;
#endif // ENABLE_LEAK_TRACKER

    void Init()
    {
    }

    void Update()
    {
        ms_linear.Clear();
    }

    void Shutdown()
    {
#if ENABLE_LEAK_TRACKER
        ms_tracker.ListLeaks();
#endif // ENABLE_LEAK_TRACKER

        ms_linear.Clear();
        ms_stack.Clear();
        ms_pool.Clear();
    }

    // ------------------------------------------------------------------------

    void* Alloc(AllocType type, i32 bytes)
    {
        ASSERT(bytes >= 0);

        void* ptr = 0;

        switch (type)
        {
        case Alloc_Stdlib:
            ptr = ms_stdlib.Alloc(bytes);
            break;
        case Alloc_Linear:
            ptr = ms_linear.Alloc(bytes);
            break;
        case Alloc_Stack:
            ptr = ms_stack.Alloc(bytes);
            break;
        case Alloc_Pool:
            ptr = ms_pool.Alloc(bytes);
            break;
        case Alloc_Debug:
            ptr = ms_debug.Alloc(bytes);
            break;
        default:
            ASSERT(false);
        }

#if ENABLE_LEAK_TRACKER
        if (type != Alloc_Debug)
        {
            ms_tracker.OnAlloc(ptr, bytes);
        }
#endif // ENABLE_LEAK_TRACKER

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

        void* ptr = 0;

        switch (hdr->type)
        {
        case Alloc_Stdlib:
            ptr = ms_stdlib.Realloc(prev, bytes);
            break;
        case Alloc_Linear:
            ptr = ms_linear.Realloc(prev, bytes);
            break;
        case Alloc_Stack:
            ptr = ms_stack.Realloc(prev, bytes);
            break;
        case Alloc_Pool:
            ptr = ms_pool.Realloc(prev, bytes);
            break;
        case Alloc_Debug:
            ptr = ms_debug.Realloc(prev, bytes);
            break;
        default:
            ASSERT(false);
        }

#if ENABLE_LEAK_TRACKER
        if (iType != Alloc_Debug)
        {
            ms_tracker.OnRealloc(prev, ptr, bytes);
        }
#endif // ENABLE_LEAK_TRACKER

        return ptr;
    }

    void Free(void* ptr)
    {
        if (ptr)
        {
            Header* hdr = (Header*)ptr;
            hdr -= 1;

            switch (hdr->type)
            {
            case Alloc_Stdlib:
                ms_stdlib.Free(ptr);
                break;
            case Alloc_Linear:
                ms_linear.Free(ptr);
                break;
            case Alloc_Stack:
                ms_stack.Free(ptr);
                break;
            case Alloc_Pool:
                ms_pool.Free(ptr);
                break;
            case Alloc_Debug:
                ms_debug.Free(ptr);
                break;
            default:
                ASSERT(false);
            }

#if ENABLE_LEAK_TRACKER
            if (hdr->type != Alloc_Debug)
            {
                ms_tracker.OnFree(ptr);
            }
#endif // ENABLE_LEAK_TRACKER
        }
    }
};
