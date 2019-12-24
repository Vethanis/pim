#pragma once

#include "common/minmax.h"
#include "common/int_types.h"

namespace Allocator
{
    void Init();
    void Update();
    void Shutdown();

    void* Alloc(AllocType type, i32 bytes);
    void Free(void* prev);
    void* Realloc(AllocType type, void* prev, i32 bytes);

    template<typename T>
    inline T* Reserve(
        AllocType type,
        T* prev,
        i32& capacity,
        i32 count)
    {
        if (count > capacity)
        {
            capacity = Max(count, Max(capacity * 2, 16));
            return (T*)Realloc(type, prev, capacity * sizeof(T));
        }
        return prev;
    }

    inline void* ImGuiAllocFn(size_t sz, void*) { return Alloc(Alloc_Pool, (i32)sz); }
    inline void ImGuiFreeFn(void* ptr, void*) { Free(ptr); }
};
