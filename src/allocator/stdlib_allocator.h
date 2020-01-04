#pragma once

#include "common/macro.h"
#include "allocator/allocator_vtable.h"
#include <stdlib.h>

namespace Allocator
{
    struct Stdlib
    {
        void Init(void* memory, i32 bytes) { }
        void Shutdown() { }
        void Clear() { }

        Header* Alloc(i32 count)
        {
            Header* hdr = (Header*)malloc(count);
            ASSERT(hdr);
            return hdr;
        }

        Header* Realloc(Header* prev, i32 count)
        {
            Header* hdr = (Header*)realloc(prev, count);
            ASSERT(hdr);
            return hdr;
        }

        void Free(Header* prev)
        {
            ASSERT(prev);
            free(prev);
        }

        static constexpr const VTable Table = VTable::Create<Stdlib>();
    };
};
