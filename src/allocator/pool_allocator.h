#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "allocator/allocator_vtable.h"
#include <string.h>

namespace Allocator
{
    struct Pool
    {
        static i32 TableBytes(i32 totalBytes)
        {
            // 1/128 of the memory is used for a lookup table.
            // This is less flexible than embedded headers,
            // but offers much better cache coherency
            return totalBytes >> 7;
        }

        static i32 TableCapacity(i32 totalBytes)
        {
            // 'sizeof(i32) * 2' == '8'
            // 'x / 8' == 'x >> 3'
            return TableBytes(totalBytes) >> 3;
        }

        struct FreeTable
        {
            i32* m_sizes;
            i32* m_offsets;
            i32 m_length;
            i32 m_capacity;

            void Clear(i32 bytes)
            {
                m_length = 1;
                m_sizes[0] = bytes - TableBytes(bytes);
                m_offsets[0] = 0;
            }

            void Init(void* memory, i32 bytes)
            {
                m_capacity = TableCapacity(bytes);
                ASSERT(m_capacity > 0);

                m_sizes = (i32*)memory;
                m_offsets = m_sizes + m_capacity;
                Clear(bytes);
            }

            inline static i32 Find(
                const i32* sizes,
                i32 count,
                i32 reqBytes)
            {
                i32 iChosen = -1;
                i32 chosenSize = 0x7fffffff;
                ASSERT(reqBytes < chosenSize);

                for (i32 i = 0; i < count; ++i)
                {
                    const i32 size = sizes[i];
                    if (size >= reqBytes)
                    {
                        if (size < chosenSize)
                        {
                            iChosen = i;
                            chosenSize = size;
                        }
                    }
                }

                return iChosen;
            }

            bool Alloc(i32 reqBytes, i32& sizeOut, i32& offsetOut)
            {
                sizeOut = 0;
                offsetOut = 0;

                const i32 i = Find(m_sizes, m_length, reqBytes);
                if (i == -1)
                {
                    return false;
                }

                i32* sizes = m_sizes;
                i32* offsets = m_offsets;

                i32 size = sizes[i];
                i32 offset = offsets[i];
                ASSERT(size >= reqBytes);

                const i32 back = --m_length;
                sizes[i] = sizes[back];
                offsets[i] = offsets[back];

                const i32 threshold = reqBytes << 1;
                ASSERT(threshold > 0);

                if (size >= threshold)
                {
                    const i32 remSize = size - reqBytes;
                    const i32 remOffset = offset + reqBytes;
                    size = reqBytes;

                    sizes[back] = remSize;
                    offsets[back] = remOffset;
                    ++m_length;
                }

                sizeOut = size;
                offsetOut = offset;

                return true;
            }

            bool Free(i32 size, i32 start)
            {
                i32 end = start + size;

                i32 count = m_length;
                i32* const offsets = m_offsets;
                i32* const sizes = m_sizes;

                for (i32 i = count - 1; i >= 0; --i)
                {
                    const i32 iStart = offsets[i];
                    const i32 iSize = sizes[i];
                    const i32 iEnd = iStart + iSize;

                    if (end == iStart)
                    {
                        end += iSize;
                        size += iSize;

                        --count;
                        offsets[i] = offsets[count];
                        sizes[i] = sizes[count];
                        i = count - 1;
                    }
                    else if (start == iEnd)
                    {
                        start = iStart;
                        size += iSize;

                        --count;
                        offsets[i] = offsets[count];
                        sizes[i] = sizes[count];
                        i = count - 1;
                    }
                }

                if (count < m_capacity)
                {
                    sizes[count] = size;
                    offsets[count] = start;
                    ++count;
                }
                else
                {
                    ASSERT(count == m_length);
                    return false;
                }

                m_length = count;

                return true;
            }
        };

        Slice<u8> m_memory; // all memory
        Slice<u8> m_user;   // user-available memory
        FreeTable m_table;

        void Init(void* memory, i32 bytes)
        {
            m_memory = { (u8*)memory, bytes };
            m_user = m_memory.Tail(TableBytes(bytes));
            m_table.Init(memory, bytes);
        }

        void Shutdown()
        {
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            const i32 size = m_memory.len;
            m_user = m_memory.Tail(TableBytes(size));
            m_table.Clear(size);
        }

        Header* Alloc(i32 reqBytes)
        {
            i32 size = 0;
            i32 offset = 0;
            if (m_table.Alloc(reqBytes, size, offset))
            {
                u8* ptr = m_user.ptr + offset;
                Header* hdr = (Header*)ptr;
                hdr->c = size;
                hdr->d = offset;
                return hdr;
            }
            else
            {
                return 0;
            }
        }

        void Free(Header* prev)
        {
            const i32 size = prev->c;
            const i32 offset = prev->d;
            ASSERT(offset >= 0);
            ASSERT(size >= prev->size);
            ASSERT((m_user.ptr + offset) == ((u8*)prev));

            bool didFree = m_table.Free(size, offset);
            ASSERT(didFree);
        }

        Header* Realloc(Header* prev, i32 reqBytes)
        {
            Slice<u8> prevSlice = { (u8*)prev, prev->size };

            Free(prev);
            Header* hdr = Alloc(reqBytes);

            if (hdr != prev)
            {
                Slice<u8> newSlice = { (u8*)hdr, hdr->size };

                const i32 offset = sizeof(Header);
                const i32 cpySize = Min(newSlice.len, prevSlice.len) - offset;
                ASSERT(cpySize > 0);

                void* dst = newSlice.ptr + offset;
                void* src = prevSlice.ptr + offset;

                if (newSlice.Overlaps(prevSlice))
                {
                    memmove(dst, src, cpySize);
                }
                else
                {
                    memcpy(dst, src, cpySize);
                }
            }

            return hdr;
        }

        static constexpr const VTable Table = VTable::Create<Pool>();
    };
};
