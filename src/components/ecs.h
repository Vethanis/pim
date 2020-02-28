#pragma once

#include "components/entity.h"
#include "components/component_id.h"
#include "allocator/allocator.h"
#include "containers/slice.h"
#include "containers/array.h"
#include "containers/bitfield.h"
#include "threading/task.h"
#include <initializer_list>

namespace ECS
{
    static constexpr i32 kMaxTypes = 256;
    using TypeFlags = BitField<ComponentId, kMaxTypes>;

    i32 EntityCount();

    Entity Create(std::initializer_list<ComponentId> components);
    bool Destroy(Entity entity);
    bool IsCurrent(Entity entity);

    bool Has(Entity entity, ComponentId type);
    bool Add(Entity entity, ComponentId type);
    bool Remove(Entity entity, ComponentId type);
    void* Get(Entity entity, ComponentId type);
    Slice<u8> GetRow(ComponentId type);

    template<typename T>
    bool Has(Entity entity) { return Has(entity, GetId<T>()); }

    template<typename T>
    bool Add(Entity entity) { return Add(entity, GetId<T>()); }

    template<typename T>
    bool Remove(Entity entity) { return Remove(entity, GetId<T>()); }

    template<typename T>
    T* Get(Entity entity) { return (T*)Get(entity, GetId<T>()); }

    template<typename T>
    Slice<T> GetRow()
    {
        return GetRow(GetId<T>()).Cast<T>();
    }

    template<typename T>
    T* Get(Entity entity, ComponentId type)
    {
        ASSERT(GetId<T>() == type);
        return (T*)Get(entity, type);
    }

    struct ForEachTask : ITask
    {
        ForEachTask() : ITask(0, 0, 1) {}

        void SetQuery(
            std::initializer_list<ComponentId> all,
            std::initializer_list<ComponentId> none);
        void GetQuery(
            Slice<const ComponentId>& all,
            Slice<const ComponentId>& none) const;
        void Execute(i32, i32) final;
        virtual void OnEntities(Slice<const Entity> entities) = 0;

    private:
        Array<ComponentId> m_all;
        Array<ComponentId> m_none;
    };
};

template<typename T>
struct CType final
{
    CType() : m_type(ECS::GetId<T>()) {}

    bool Add(Entity entity) const
    {
        return ECS::Add(entity, m_type);
    }
    bool Remove(Entity entity) const
    {
        return ECS::Remove(entity, m_type);
    }
    T* Get(Entity entity) const
    {
        return (T*)ECS::Get(entity, m_type);
    }
    Slice<T> GetRow() const
    {
        return ECS::GetRow(m_type).Cast<T>();
    }

private:
    ComponentId m_type;
};
