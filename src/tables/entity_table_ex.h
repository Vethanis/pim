#pragma once

#include "tables/entity_table.h"
#include "common/reflection.h"

namespace EntityTableEx
{
    template<typename T>
    inline void Register(void(*OnAdd)(T*), void(*OnRemove)(T*))
    {
        using VoidpVoidFn = void(*)(void*);
        EntityTable::Register(RfId<T>(), (VoidpVoidFn)OnAdd, (VoidpVoidFn)OnRemove);
    }
    template<typename T>
    inline Slice<const u32> CompVersions(u8 table)
    {
        return EntityTable::CompVersions(table, RfId<T>());
    }
    template<typename T>
    inline Slice<T> CompData(u8 table)
    {
        return EntityTable::CompData(table, RfId<T>()).cast<T>();
    }
    template<typename T>
    inline void Add(Entity entity)
    {
        EntityTable::Add(entity, RfId<T>());
    }
    template<typename T>
    inline void Remove(Entity entity)
    {
        EntityTable::Remove(entity, RfId<T>());
    }
    template<typename T>
    inline T* Get(Entity entity)
    {
        return (T*)EntityTable::Get(entity, RfId<T>());
    }
};
