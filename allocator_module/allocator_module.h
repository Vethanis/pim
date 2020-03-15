#pragma once

#include "common/macro.h"

#ifdef ALLOC_MODULE
#define ALLOC_API PIM_EXPORT
#else
#define ALLOC_API PIM_IMPORT
#endif // ALLOC_MODULE

PIM_C_BEGIN

#include <stdint.h>

typedef enum EAlloc
{
    EAlloc_Init = 0,
    EAlloc_Perm,
    EAlloc_Temp,
    EAlloc_Thread,
    EAlloc_Count
} EAllocator;

typedef struct
{
    void  (PIM_CDECL *Init)(const int32_t* sizes, int32_t count);
    void  (PIM_CDECL *Update)(void);
    void  (PIM_CDECL *Shutdown)(void);
    void* (PIM_CDECL *Alloc)(EAllocator type, int32_t bytes);
    void  (PIM_CDECL *Free)(void* ptr);
} allocator_module_t;

ALLOC_API const allocator_module_t* PIM_CDECL allocator_module_export(void);

PIM_C_END
