#pragma once

#include "common/typeid.h"

struct ComponentId
{
    i32 Value;

    inline operator i32 () const { return Value; }
};

namespace ComponentRegistry
{
    void Register(ComponentId id, i32 size);
    i32 Size(ComponentId id);
};

template<typename ComponentType>
struct ComponentMeta
{
    static ComponentId ms_id;
    static i32 ms_size;

    ComponentMeta()
    {
        ms_id.Value = TypeIds<ComponentId>::Get<ComponentType>();
        ms_size = sizeof(ComponentType);
        ComponentRegistry::Register(ms_id, ms_size);
    }

    static ComponentId Id() { return ms_id; }
    static i32 Size() { return ms_size; }
};

template<typename ComponentType>
ComponentId ComponentMeta<ComponentType>::ms_id;

template<typename ComponentType>
i32 ComponentMeta<ComponentType>::ms_size;
