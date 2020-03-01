#pragma once

#include "common/int_types.h"
#include "common/guid.h"
#include "common/type_guid.h"

struct ComponentId
{
    i32 Value;
};

static bool operator==(ComponentId lhs, ComponentId rhs) { return lhs.Value == rhs.Value; }
static bool operator!=(ComponentId lhs, ComponentId rhs) { return lhs.Value == rhs.Value; }
static bool operator<(ComponentId lhs, ComponentId rhs) { return lhs.Value < rhs.Value; }

namespace ECS
{
    ComponentId RegisterType(Guid guid, i32 sizeOf);

    template<typename T>
    static ComponentId GetId()
    {
        static const i32 index = RegisterType(TGuidOf<T>(), sizeof(T)).Value;
        return { index };
    }
};

template<typename T>
static ComponentId CTypeOf()
{
    return ECS::GetId<T>();
}
