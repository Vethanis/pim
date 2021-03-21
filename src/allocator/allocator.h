#pragma once

#include "common/macro.h"

PIM_C_BEGIN

void MemSys_Init(void);
void MemSys_Update(void);
void MemSys_Shutdown(void);

void Mem_Free(void* ptr);
void* Mem_Alloc(EAlloc allocator, i32 bytes);
void* Mem_Realloc(EAlloc allocator, void* prev, i32 bytes);
void* Mem_Calloc(EAlloc allocator, i32 bytes);

// alternative to alloca
void* Mem_Push(i32 bytes);
void Mem_Pop(i32 bytes);

#define Perm_Alloc(bytes) Mem_Alloc(EAlloc_Perm, (bytes))
#define Perm_Calloc(bytes) Mem_Calloc(EAlloc_Perm, (bytes))
#define Perm_Realloc(prev, bytes) Mem_Realloc(EAlloc_Perm, (prev), (bytes))

#define Tex_Alloc(bytes) Mem_Alloc(EAlloc_Texture, (bytes))
#define Tex_Calloc(bytes) Mem_Calloc(EAlloc_Texture, (bytes))
#define Tex_Realloc(prev, bytes) Mem_Realloc(EAlloc_Texture, (prev), (bytes))

#define Temp_Alloc(bytes) Mem_Alloc(EAlloc_Temp, (bytes))
#define Temp_Calloc(bytes) Mem_Calloc(EAlloc_Temp, (bytes))
#define Temp_Realloc(prev, bytes) Mem_Realloc(EAlloc_Temp, (prev), (bytes))

#define ZeroElem(p, i) do { memset((p) + (i), 0, sizeof((p)[0])); } while(0)
#define PopSwap(p, i, l) do { memcpy((p) + (i), (p) + (l) - 1, sizeof((p)[0])); } while(0)

#define Mem_Reserve(e, p, l) do { (p) = Mem_Realloc((e), (p), sizeof((p)[0]) * (l)); } while(0)

#define Mem_Grow(e, p, l) do { Mem_Reserve((e), (p), (l)); ZeroElem((p), (l) - 1); } while(0)

#define Perm_Reserve(p, l) Mem_Reserve(EAlloc_Perm, (p), (l))
#define Perm_Grow(p, l) Mem_Grow(EAlloc_Perm, (p), (l))

#define Temp_Reserve(p, l) Mem_Reserve(EAlloc_Temp, (p), (l))
#define Temp_Grow(p, l) Mem_Grow(EAlloc_Temp, (p), (l))

PIM_C_END
