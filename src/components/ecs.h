#pragma once

#include "components/entity.h"
#include "common/typeid.h"
#include "containers/initlist.h"
#include "allocator/allocator.h"

namespace Ecs
{
    Entity Create();
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    void* Get(Entity entity, TypeId type);
    void* Add(Entity entity, TypeId type);
    bool Remove(Entity entity, TypeId type);

    template<typename T>
    T* Get(Entity entity)
    {
        return (T*)Get(entity, TypeOf<T>());
    }

    template<typename T>
    T* Add(Entity entity)
    {
        return (T*)Add(entity, TypeOf<T>());
    }

    template<typename T>
    bool Remove(Entity entity)
    {
        return Remove(entity, TypeOf<T>());
    }

    struct QueryResult
    {
        QueryResult(Entity* pEntities, i32 numEntities) : ptr(pEntities), length(numEntities) {}
        ~QueryResult() { Allocator::Free(ptr); ptr = 0; length = 0; }
        const Entity* begin() const { return ptr; }
        const Entity* end() const { return ptr + length; }
    private:
        Entity* ptr;
        i32 length;
    };

    QueryResult ForEach(std::initializer_list<TypeId> all, std::initializer_list<TypeId> none = {});
    QueryResult ForEach(const TypeId* pAll, i32 allCount, const TypeId* pNone, i32 noneCount);
};
