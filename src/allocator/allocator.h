#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef enum
{
    EAlloc_Init = 0,
    EAlloc_Perm,
    EAlloc_Temp,
    EAlloc_TLS,
    EAlloc_Count
} EAlloc;

typedef struct allocator_s
{
    void  (PIM_CDECL *Init)(void);
    void  (PIM_CDECL *Update)(void);
    void  (PIM_CDECL *Shutdown)(void);
    void* (PIM_CDECL *Alloc)(EAlloc type, int32_t bytes);
    void  (PIM_CDECL *Free)(void* ptr);
    void* (PIM_CDECL *Realloc)(EAlloc type, void* prev, int32_t bytes);
    void* (PIM_CDECL *Calloc)(EAlloc type, int32_t bytes);
} allocator_t;

extern const allocator_t CAllocator;

PIM_C_END

#ifdef __cplusplus

namespace Allocator
{
    template<typename T>
    static T* AllocT(EAlloc type, int32_t count)
    {
        return reinterpret_cast<T*>(CAllocator.Alloc(type, sizeof(T) * count));
    }

    template<typename T>
    static T* ReallocT(EAlloc type, T* prev, int32_t count)
    {
        return reinterpret_cast<T*>(CAllocator.Realloc(type, prev, sizeof(T) * count));
    }

    template<typename T>
    static T* CallocT(EAlloc type, int32_t count)
    {
        return reinterpret_cast<T*>(CAllocator.Calloc(type, sizeof(T) * count));
    }
};

#endif // __cplusplus
