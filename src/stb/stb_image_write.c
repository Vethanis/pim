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
#include "miniz/miniz.h"

static unsigned char* Stbiw_Compress(
    unsigned char* pInput,
    int inputSize,
    int *outputSizeOut,
    int quality)
{
    ASSERT(pInput);
    ASSERT(inputSize > 0);
    ASSERT(outputSizeOut);
    ASSERT(quality >= 0);
    unsigned char* output = Tex_Alloc(inputSize);
    mz_ulong outputLen = (mz_ulong)inputSize;
    int status = mz_compress2(
        output, &outputLen, pInput, (mz_ulong)inputSize, MZ_BEST_COMPRESSION);
    ASSERT(status == MZ_OK);
    if (status != MZ_OK)
    {
        Mem_Free(output);
        output = NULL;
        outputLen = 0;
    }
    *outputSizeOut = (int)outputLen;
    ASSERT(*outputSizeOut >= 0);
    return output;
}

#define STBIW_ASSERT(x)             ASSERT(x)
#define STBIW_MALLOC(bytes)         Tex_Alloc(bytes)
#define STBIW_REALLOC(ptr, bytes)   Tex_Realloc(ptr, bytes)
#define STBIW_FREE(ptr)             Mem_Free(ptr)
#define STBIW_ZLIB_COMPRESS         Stbiw_Compress

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
