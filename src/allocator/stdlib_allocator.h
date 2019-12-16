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
            hdr->type = Alloc_Stdlib;
            hdr->size = count;
            return hdr;
        }

        Header* Realloc(Header* prev, i32 count)
        {
            Header* hdr = (Header*)realloc(prev, count);
            ASSERT(hdr);
            hdr->size = count;
            hdr->type = Alloc_Stdlib;
            return hdr;
        }

        void Free(Header* prev)
        {
            ASSERT(prev);
            ASSERT(prev->type == Alloc_Stdlib);
            free(prev);
        }

        static constexpr const VTable Table = VTable::Create<Stdlib>();
    };
};
