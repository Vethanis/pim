#include "common/allocator.h"
#include "macro.h"
#include <malloc.h>
#include <string.h>

namespace Allocator
{
    constexpr usize DefaultAlignment = 64;

    void* _Alloc(AllocatorType type, usize bytes)
    {
        void* ptr = 0;
        if (bytes)
        {
            ptr = _aligned_malloc(bytes, DefaultAlignment);
            DebugAssert(ptr);
            if (ptr)
            {
                memset(ptr, 0, bytes);
            }
        }
        return ptr;
    }

    void* _Realloc(AllocatorType type, void* pPrev, usize prevBytes, usize newBytes)
    {
        if (newBytes == 0)
        {
            _Free(type, pPrev);
            return 0;
        }
        if (newBytes > prevBytes)
        {
            void* ptr = _aligned_realloc(pPrev, newBytes, DefaultAlignment);
            DebugAssert(ptr);
            if (ptr)
            {
                memset((u8*)ptr + prevBytes, 0, newBytes - prevBytes);
            }
            return ptr;
        }
        return pPrev;
    }

    void _Free(AllocatorType type, void* pPrev)
    {
        if(pPrev)
        {
            _aligned_free(pPrev);
        }
    }
};
