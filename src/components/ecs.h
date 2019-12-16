#pragma once

#include "common/macro.h"
#include "common/int_types.h"
#include "common/guid.h"
#include "common/hashstring.h"
#include "common/sort.h"
#include "common/find.h"

#include "containers/initlist.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/dict.h"
#include "containers/generational.h"

#include "components/entity.h"
#include "components/component.h"

namespace Ecs
{
    void Init();
    void Shutdown();

    // ------------------------------------------------------------------------

    struct ComponentInfo
    {
        ComponentId* ids;   // id of each component, sorted (required for deterministic guids from hash)
        i32* strides;       // size of each component
        i32 count;          // number of components in the archetype
        i32 bytesPerEntity; // bytes required for each entity
    };

    struct ChunkInfo
    {
        i32* offsets;       // offset to each component lane
        i32 count;          // number of resident entities
        i32 capacity;       // number of entities this chunk can hold
        i32 entityOffset;   // 0th entity will have this index
    };

    // dynamic struct Chunk
    // {
    //     T0 t0s[capacity];
    //     T1 t1s[capacity];
    //     etc
    // };
    struct Chunk { };

    struct Archetype
    {
        GenIdAllocator entAllocator;    // entity allocator
        ComponentInfo compInfo;         // component info
        Array<ChunkInfo> chunkInfos;    // chunk infos
        Array<Chunk*> chunks;           // chunk pointers
        i32 entityBegin;                // 0th entity of entAllocator will have this index
        i32 entityEnd;
    };

    struct Archetypes
    {
        DictTable<64, Guid, Archetype> table;
    };

    // ------------------------------------------------------------------------

    static Guid ToGuid(const ComponentId* ids, i32 count)
    {
        return Guid(ids, sizeof(ComponentId) * count);
    }

    static void Init(
        ComponentInfo& info,
        Slice<const ComponentId> ids)
    {
        info = {};
        info.count = ids.Size();

        info.ids = (ComponentId*)Allocator::Alloc(
            Alloc_Pool, ids.Bytes());
        memcpy(info.ids, ids.begin(), ids.Bytes());
        Sort(info.ids, info.count);

        info.strides = (i32*)Allocator::Alloc(
            Alloc_Pool, sizeof(i32) * info.count);

        info.bytesPerEntity = 0;
        for (i32 i = 0; i < info.count; ++i)
        {
            info.strides[i] = ComponentRegistry::Size(info.ids[i]);
            info.bytesPerEntity += info.strides[i];
        }
    }

    static Guid Init(
        Archetype& arch,
        Slice<const ComponentId> ids,
        i32 entityBegin,
        i32 entityEnd)
    {
        arch = {};
        arch.entAllocator.Init(Alloc_Pool);
        arch.chunkInfos.Init(Alloc_Pool);
        arch.chunks.Init(Alloc_Pool);
        arch.entityBegin = entityBegin;
        arch.entityEnd = entityEnd;
        Init(arch.compInfo, ids);
        return ToGuid(arch.compInfo.ids, arch.compInfo.count);
    }

    static void Init(
        ChunkInfo& chunkInfo,
        ComponentInfo compInfo,
        i32 capacity,
        i32 entityOffset)
    {
        chunkInfo = {};
        chunkInfo.capacity = capacity;
        chunkInfo.count = 0;
        chunkInfo.entityOffset = entityOffset;
        chunkInfo.offsets = (i32*)Allocator::Alloc(
            Alloc_Pool, compInfo.count * sizeof(i32));
        i32 offset = 0;
        for (i32 i = 0; i < compInfo.count; ++i)
        {
            chunkInfo.offsets[i] = offset;
            offset += capacity * compInfo.strides[i];
        }
    }

    static Chunk* AddChunk(Archetype& arch, i32 capacity)
    {
        i32 entityOffset = arch.entityBegin;
        for (const ChunkInfo& info : arch.chunkInfos)
        {
            entityOffset += info.capacity;
        }
        ASSERT(entityOffset < arch.entityEnd);

        Init(arch.chunkInfos.Grow(), arch.compInfo, capacity, entityOffset);

        const i32 bytes = capacity * arch.compInfo.bytesPerEntity;
        Chunk* chunk = (Chunk*)Allocator::Alloc(Alloc_Pool, bytes);
        memset(chunk, 0, bytes);
        arch.chunks.Grow() = chunk;

        return chunk;
    }

    // ------------------------------------------------------------------------

    static i32 FindRow(const Archetype& arch, ComponentId id)
    {
        return Find(arch.compInfo.ids, arch.compInfo.count, id);
    }

    static i32 FindChunk(const Archetype& arch, Entity entity)
    {
        const ChunkInfo* infos = arch.chunkInfos.begin();
        const i32 count = arch.chunkInfos.Size();
        for (i32 i = 0; i < count; ++i)
        {
            const i32 start = infos[i].entityOffset;
            const i32 end = start + infos[i].capacity;
            if ((entity.index >= start) &&
                (entity.index < end))
            {
                return i;
            }
        }
        return -1;
    }

    static i32 FindArchetype(Slice<const Archetype> archs, Entity entity)
    {
        for (i32 i = 0; i < archs.len; ++i)
        {
            const Archetype& arch = archs[i];
            if ((entity.index >= arch.entityBegin) &&
                (entity.index < arch.entityEnd))
            {
                return i;
            }
        }
        return -1;
    }

    // ------------------------------------------------------------------------

    static u8* GetComponents(Chunk* chunk, ChunkInfo chunkInfo, i32 row)
    {
        return reinterpret_cast<u8*>(chunk) + chunkInfo.offsets[row];
    }

    static u8* GetComponents(Archetype& arch, i32 iChunk, i32 row)
    {
        return GetComponents(arch.chunks[iChunk], arch.chunkInfos[iChunk], row);
    }

    static u8* GetComponents(Archetype& arch, i32 iChunk, ComponentId id)
    {
        i32 row = FindRow(arch, id);
        if (row == -1)
        {
            return 0;
        }
        return GetComponents(arch, iChunk, row);
    }

    // ------------------------------------------------------------------------
};
