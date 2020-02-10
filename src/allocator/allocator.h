#pragma once

#include "common/int_types.h"

namespace Allocator
{
    void Init();
    void Update();
    void Shutdown();

    void* Alloc(AllocType type, i32 bytes);
    void Free(void* prev);
    void* Realloc(AllocType type, void* prev, i32 bytes);
    void* Calloc(AllocType type, i32 bytes);

    template<typename T>
    inline T* AllocT(AllocType type, i32 count)
    {
        return (T*)Alloc(type, sizeof(T) * count);
    }

    template<typename T>
    inline T* ReallocT(AllocType type, T* prev, i32 count)
    {
        return (T*)Realloc(type, prev, sizeof(T) * count);
    }

    template<typename T>
    inline T* CallocT(AllocType type, i32 count)
    {
        return (T*)Calloc(type, sizeof(T) * count);
    }

    inline void* ImGuiAllocFn(size_t sz, void*) { return Alloc(Alloc_Tlsf, (i32)sz); }
    inline void ImGuiFreeFn(void* ptr, void*) { Free(ptr); }
};
