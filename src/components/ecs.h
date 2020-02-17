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

    struct Selection
    {
        Selection(Entity* pEntities, i32 numEntities) : ptr(pEntities), length(numEntities) {}
        ~Selection() { Allocator::Free(ptr); ptr = 0; length = 0; }
        const Entity* begin() const { return ptr; }
        const Entity* end() const { return ptr + length; }
        i32 size() const { return length; }
        Entity operator[](i32 i) const
        {
            ASSERT((u32)i < (u32)length);
            return ptr[i];
        }
    private:
        Entity* ptr;
        i32 length;
    };

    Selection Select(std::initializer_list<ComponentType> all = {}, std::initializer_list<ComponentType> none = {});
    Selection Select(Slice<const ComponentType> all, Slice<const ComponentType> none);

    struct RowBorrow final
    {
        RowBorrow(ComponentType type);
        ~RowBorrow();
        i32 Has(Entity entity) const;
        i32 Next(const Selection& query, i32 i) const;
        const void* GetPtrR(Entity entity) const;
        void* GetPtrRW(Entity entity);

        template<typename T>
        const T& GetR(Entity entity) const
        {
            ASSERT(sizeof(T) == m_stride);
            ASSERT(GetId<T>() == m_type.id);
            ASSERT(Has(entity));
            return reinterpret_cast<const T*>(m_bytes)[entity.index];
        }

        template<typename T>
        T& GetRW(Entity entity)
        {
            ASSERT(sizeof(T) == m_stride);
            ASSERT(GetId<T>() == m_type.id);
            ASSERT(m_type.write);
            ASSERT(Has(entity));
            return reinterpret_cast<T*>(m_bytes)[entity.index];
        }

    private:
        RowBorrow(const RowBorrow&) = delete;
        RowBorrow& operator=(const RowBorrow&) = delete;

        ComponentType m_type;
        const i32* m_versions;
        u8* m_bytes;
        i32 m_stride;
    };
};
