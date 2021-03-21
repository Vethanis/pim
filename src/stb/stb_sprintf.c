#include "common/macro.h"
#include "allocator/allocator.h"

#define STBI_ASSERT(x)              ASSERT(x)
#define STBI_MALLOC(bytes)          Tex_Alloc((i32)(bytes))
#define STBI_REALLOC(prev, bytes)   Tex_Realloc(prev, (i32)(bytes))
#define STBI_FREE(ptr)              Mem_Free(ptr)

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>
