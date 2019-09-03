#pragma once

#include "tables/entity.h"
#include "containers/slice.h"

namespace EntityTable
{
    void Init();
    void Shutdown();
    void Visualize();

    void Register(
        usize typeId,
        void(*OnAdd)(void*),
        void(*OnRemove)(void*));

    Slice<const u32> EntVersions(u8 table);
    Slice<const u32> CompVersions(u8 table, usize typeId);
    Slice<u8> CompData(u8 table, usize typeId);

    inline bool Alive(u32 version) { return version & 1u; }
    bool Alive(Entity entity);
    Entity Create(u8 table);
    void Destroy(Entity entity);

    void Add(Entity entity, usize typeId);
    void Remove(Entity entity, usize typeId);
    void* Get(Entity entity, usize typeId);
};
