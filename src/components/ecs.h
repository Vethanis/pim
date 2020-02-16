#pragma once

#include "components/entity.h"
#include "components/component_id.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include <initializer_list>

namespace ECS
{
    Entity Create();
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    bool Add(Entity entity, ComponentId type);
    bool Remove(Entity entity, ComponentId type);
    bool Get(Entity entity, ComponentId type, void* dst, i32 sizeOf);
    bool Set(Entity entity, ComponentId type, const void* src, i32 sizeOf);

    template<typename T>
    bool Add(Entity entity) { return Add(entity, GetId<T>()); }

    template<typename T>
    bool Remove(Entity entity) { return Remove(entity, GetId<T>()); }

    template<typename T>
    bool Get(Entity entity, T& valueOut)
    {
        return Get(entity, GetId<T>(), &valueOut, sizeof(T));
    }

    template<typename T>
    bool Set(Entity entity, const T& valueIn)
    {
        return Set(entity, GetId<T>(), &valueIn, sizeof(T));
    }

    struct QueryResult
    {
        QueryResult(Entity* pEntities, i32 numEntities) : ptr(pEntities), length(numEntities) {}
        ~QueryResult() { Allocator::Free(ptr); ptr = 0; length = 0; }
        const Entity* begin() const { return ptr; }
        const Entity* end() const { return ptr + length; }
        i32 size() const { return length; }
    private:
        Entity* ptr;
        i32 length;
    };

    QueryResult ForEach(std::initializer_list<ComponentType> all = {}, std::initializer_list<ComponentType> none = {});
    QueryResult ForEach(Slice<const ComponentType> all, Slice<const ComponentType> none);
};
