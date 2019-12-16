#pragma once

#include "common/macro.h"
#include "allocator/allocator_vtable.h"
#include "containers/slice.h"

namespace Allocator
{
    struct Stack
    {
        Slice<u8> m_memory;
        Slice<u8> m_stack;

        void Init(void* memory, i32 bytes)
        {
            m_memory = { (u8*)memory, bytes };
            m_stack = m_memory;
        }

        void Shutdown()
        {
            m_memory = { 0, 0 };
            m_stack = { 0, 0 };
        }

        void Clear()
        {
            m_stack = m_memory;
        }

        Header* Alloc(i32 count)
        {
            Slice<u8> allocation = m_stack.Head(count);
            m_stack = m_stack.Tail(count);
            return (Header*)allocation.ptr;
        }

        Header* Realloc(Header* hdr, i32 count)
        {
            Free(hdr);
            return Alloc(count);
        }

        void Free(Header* hdr)
        {
            Slice<u8> prev = { (u8*)hdr, hdr->size };
            u8* top = m_stack.ptr;
            u8* end = prev.end();

            if (end == top)
            {
                // free the top of the stack.
                // this matches the API usage of a stack.
                m_stack.ptr = prev.ptr;
                m_stack.len = m_memory.len - (i32)(prev.ptr - m_memory.ptr);
            }
            else if (end > top)
            {
                // allocation is above the stack.
                // this is not a LIFO usage pattern.
                ASSERT(false);
            }
            else
            {
                // allocation is beneath the top of the stack.
                // this is not a LIFO usage pattern.
                ASSERT(false);
            }
        }

        static constexpr const VTable Table = VTable::Create<Stack>();
    };
};
