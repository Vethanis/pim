#pragma once

#include "common/macro.h"
#include "common/minmax.h"
#include "containers/slice.h"
#include "allocator/allocator_vtable.h"
#include <string.h>
#include <stdlib.h>

namespace Allocator
{
    struct Pool
    {
        struct FreeTable
        {
            i32* m_sizes;
            i32* m_offsets;
            i32 m_length;
            i32 m_capacity;

            void Clear(i32 bytes)
            {
                ASSERT(bytes > 0);

                m_length = 1;
                if (m_capacity == 0)
                {
                    constexpr i32 StartCap = 64;
                    constexpr i32 StartBytes = StartCap * sizeof(i32);

                    m_sizes = (i32*)malloc(StartBytes);
                    m_offsets = (i32*)malloc(StartBytes);
                    ASSERT(m_sizes);
                    ASSERT(m_offsets);
                    m_capacity = StartCap;
                }

                m_sizes[0] = bytes;
                m_offsets[0] = 0;
            }

            void Init(i32 bytes)
            {
                memset(this, 0, sizeof(*this));
                Clear(bytes);
            }

            void Reset()
            {
                free(m_sizes);
                free(m_offsets);
                memset(this, 0, sizeof(*this));
            }

            static i32 Find(
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
                    if ((size >= reqBytes) &&
                        (size < chosenSize))
                    {
                        iChosen = i;
                        chosenSize = size;
                    }
                }

                return iChosen;
            }

            static void Defrag(
                i32* sizes,
                i32* offsets,
                i32& rLength,
                i32& rSize,
                i32& rOffset)
            {
                i32 length = rLength;
                i32 size = rSize;
                i32 offset = rOffset;
                i32 end = offset + size;
                ASSERT(end > 0);

                for (i32 i = length - 1; i >= 0; --i)
                {
                    const i32 iSize = sizes[i];
                    const i32 iOffset = offsets[i];
                    const i32 iEnd = iOffset + iSize;
                    ASSERT(iEnd > 0);
                    ASSERT(iOffset != offset);

                    if (end == iOffset)
                    {
                        end = iEnd;
                        size += iSize;

                        --length;
                        sizes[i] = sizes[length];
                        offsets[i] = offsets[length];
                        i = length - 1;
                    }
                    else if (offset == iEnd)
                    {
                        offset = iOffset;
                        size += iSize;

                        --length;
                        sizes[i] = sizes[length];
                        offsets[i] = offsets[length];
                        i = length - 1;
                    }
                }

                rLength = length;
                rSize = size;
                rOffset = offset;
            }

            bool Alloc(i32 reqBytes, i32& sizeOut, i32& offsetOut)
            {
                sizeOut = 0;
                offsetOut = 0;

                i32* sizes = m_sizes;
                i32* offsets = m_offsets;
                i32 length = m_length;

                const i32 i = Find(
                    sizes,
                    length,
                    reqBytes);
                if (i == -1)
                {
                    return false;
                }

                i32 size = sizes[i];
                i32 offset = offsets[i];
                ASSERT(size >= reqBytes);

                const i32 back = --length;
                ASSERT(back >= 0);

                sizes[i] = sizes[back];
                offsets[i] = offsets[back];

                if (size > reqBytes)
                {
                    const i32 remSize = size - reqBytes;
                    const i32 remOffset = offset + reqBytes;
                    size = reqBytes;

                    sizes[back] = remSize;
                    offsets[back] = remOffset;
                    ++length;
                }

                m_length = length;
                sizeOut = size;
                offsetOut = offset;

                return true;
            }

            void Free(i32 size, i32 offset)
            {
                Defrag(
                    m_sizes,
                    m_offsets,
                    m_length,
                    size,
                    offset);

                const i32 back = m_length++;
                const i32 cap = m_capacity;
                if (back == cap)
                {
                    const i32 newCap = cap * 2;
                    const i32 newBytes = newCap * sizeof(i32);
                    ASSERT(newBytes > 0);

                    m_sizes = (i32*)realloc(m_sizes, newBytes);
                    m_offsets = (i32*)realloc(m_offsets, newBytes);
                    ASSERT(m_sizes);
                    ASSERT(m_offsets);
                    m_capacity = newCap;
                }

                m_sizes[back] = size;
                m_offsets[back] = offset;
            }
        };

        Slice<u8> m_memory;
        FreeTable m_table;

        void Init(void* memory, i32 bytes)
        {
            m_memory = { (u8*)memory, bytes };
            m_table.Init(bytes);
        }

        void Shutdown()
        {
            m_table.Reset();
            memset(this, 0, sizeof(*this));
        }

        void Clear()
        {
            m_table.Clear(m_memory.Size());
        }

        Header* Alloc(i32 reqBytes)
        {
            i32 size = 0;
            i32 offset = 0;
            if (m_table.Alloc(reqBytes, size, offset))
            {
                ASSERT(size >= reqBytes);
                Slice<u8> allocation = m_memory.Subslice(offset, size);
                Header* pNew = (Header*)allocation.begin();
                pNew->size = size;
                pNew->type = Alloc_Pool;
                pNew->c = size;
                pNew->d = offset;
                return pNew;
            }
            else
            {
                return nullptr;
            }
        }

        void Free(Header* prev)
        {
            const i32 size = prev->c;
            const i32 offset = prev->d;
            ASSERT(offset >= 0);
            ASSERT(size >= prev->size);
            ASSERT((m_memory.begin() + offset) == ((u8*)prev));

            m_table.Free(size, offset);
        }

        Header* Realloc(Header* pOld, i32 reqBytes)
        {
            Free(pOld);
            Header* pNew = Alloc(reqBytes);
            if (!pNew)
            {
                return nullptr;
            }

            if (pNew != pOld)
            {
                Slice<u8> oldSlice = pOld->AsSlice().Tail(sizeof(Header));
                Slice<u8> newSlice = pNew->AsSlice().Tail(sizeof(Header));
                const i32 cpySize = Min(newSlice.Size(), oldSlice.Size());
                if (newSlice.Overlaps(oldSlice))
                {
                    memmove(newSlice.begin(), oldSlice.begin(), cpySize);
                }
                else
                {
                    memcpy(newSlice.begin(), oldSlice.begin(), cpySize);
                }
            }

            return pNew;
        }

        static constexpr const VTable Table = VTable::Create<Pool>();
    };
};
