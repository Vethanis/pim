#include "allocator/allocator.h"
#include "allocator/stdlib_allocator.h"
#include "allocator/pool_allocator.h"
#include "allocator/linear_allocator.h"
#include "allocator/stack_allocator.h"
#include "containers/array.h"
#include "common/hashstring.h"
#include <string.h>

namespace Allocator
{
    static StdlibAllocator ms_stdlibAllocator;
    static PoolAllocator ms_poolAllocator;
    static LinearAllocator ms_linearAllocator;
    static StackAllocator ms_stackAllocator;

    static BaseAllocator* ms_allocators[] =
    {
        &ms_stdlibAllocator,
        &ms_poolAllocator,
        &ms_linearAllocator,
        &ms_stackAllocator,
    };

    static const HashKey ImGuiHash = HStr::Hash("ImGui");

    void* ImGuiAllocFn(size_t sz, void*)
    {
        if (sz > 0)
        {
            i32 iSize = (i32)sz;
            Assert(iSize > 0);

            Allocation alloc = Alloc(Alloc_Stdlib, iSize + 16);

            u64* pHeader = (u64*)alloc.begin();
            pHeader[0] = alloc.Size();
            pHeader[1] = ImGuiHash;

            return pHeader + 2;
        }
        return nullptr;
    }

    void ImGuiFreeFn(void* ptr, void*)
    {
        if (ptr)
        {
            u64* pHeader = (u64*)ptr - 2;
            Assert((u64)pHeader[1] == ImGuiHash);
            Allocation alloc =
            {
                (u8*)pHeader,
                (i32)pHeader[0],
                Alloc_Stdlib,
            };

            Free(alloc);
        }
    }

    // ------------------------------------------------------------------------

    void Init()
    {
        for (BaseAllocator* allocator : ms_allocators)
        {
            allocator->Init(32 << 20);
        }
    }

    void Update()
    {
        for (BaseAllocator* allocator : ms_allocators)
        {
            allocator->Clear();
        }
    }

    void Shutdown()
    {
        for (BaseAllocator* allocator : ms_allocators)
        {
            allocator->Shutdown();
        }
    }

    Allocation Alloc(AllocType type, i32 want)
    {
        Assert(want >= 0);
        Allocation got = { 0, 0, type };

        if (want > 0)
        {
            got.slice = ms_allocators[type]->Alloc(want);
            Assert(got.begin());
        }

        return got;
    }

    void Realloc(Allocation& prev, i32 want)
    {
        const i32 has = prev.Size();
        Assert(want >= 0);
        Assert(has >= 0);

        Allocation got = { 0, 0, prev.type };

        if (!prev.begin())
        {
            got = Alloc(prev.type, want);
        }
        else if (want <= 0)
        {
            Free(prev);
        }
        else if (want != has)
        {
            got.slice = ms_allocators[prev.type]->Realloc(prev.slice, want);
            Assert(got.begin());
        }
        else
        {
            got = prev;
        }

        prev = got;
    }

    void Free(Allocation& prev)
    {
        if (prev.begin())
        {
            ms_allocators[prev.type]->Free(prev.slice);
        }
        prev.slice = { 0, 0 };
    }
};
