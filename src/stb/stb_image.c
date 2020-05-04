#include "common/macro.h"
#include "allocator/allocator.h"

#define STBI_ASSERT(x)              ASSERT(x)
#define STBI_MALLOC(bytes)          pim_malloc(EAlloc_Perm, (i32)(bytes))
#define STBI_REALLOC(prev, bytes)   pim_realloc(EAlloc_Perm, prev, (i32)(bytes))
#define STBI_FREE(ptr)              pim_free(ptr)

#define STB_IMAGE_IMPLEMENTATION

#include "stb/stb_image.h"
