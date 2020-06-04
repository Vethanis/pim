//You can #define STBIW_ASSERT(x) before the #include to avoid using assert.h.
//You can #define STBIW_MALLOC(), STBIW_REALLOC(), and STBIW_FREE() to replace
//malloc, realloc, free.
//You can #define STBIW_MEMMOVE() to replace memmove()
//You can #define STBIW_ZLIB_COMPRESS to use a custom zlib - style compress function
//for PNG compression(instead of the builtin one), it must have the following signature :
//unsigned char * my_compress(unsigned char *data, int data_len, int *out_len, int quality);
//The returned data will be freed with STBIW_FREE() (free() by default),
//so it must be heap allocated with STBIW_MALLOC() (malloc() by default),

#include "common/macro.h"
#include "allocator/allocator.h"

#define STBIW_ASSERT(x) ASSERT(x)
#define STBIW_MALLOC(bytes) perm_malloc(bytes)
#define STBIW_REALLOC(ptr, bytes) perm_realloc(ptr, bytes)
#define STBIW_FREE(ptr) pim_free(ptr)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
