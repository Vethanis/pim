#include "common/allocator.h"
#include "macro.h"
#include <malloc.h>

namespace Allocator
{
    constexpr usize DefaultAlignment = 64;

    void* _Alloc(AllocatorType type, usize bytes)
    {
        void* ptr = _aligned_malloc(bytes, DefaultAlignment);
        DebugAssert(ptr);
        return ptr;
    }

    void* _Realloc(AllocatorType type, void* pPrev, usize prevBytes, usize newBytes)
    {
        void* ptr = _aligned_realloc(pPrev, newBytes, DefaultAlignment);
        DebugAssert(ptr || !newBytes);
        return ptr;
    }

    void _Free(AllocatorType type, void* pPrev)
    {
        if(pPrev)
        {
            _aligned_free(pPrev);
        }
    }
};
