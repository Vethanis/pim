#include "common/macro.h"
#include "allocator/allocator.h"

#define STBI_ASSERT(x)              ASSERT(x)
#define STBI_MALLOC(bytes)          tex_malloc((i32)(bytes))
#define STBI_REALLOC(prev, bytes)   tex_realloc(prev, (i32)(bytes))
#define STBI_FREE(ptr)              pim_free(ptr)

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>
