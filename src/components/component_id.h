#pragma once

#include "common/int_types.h"
#include "common/guid.h"

struct ComponentId
{
    i32 Value;
};

struct ComponentType
{
    ComponentId id;
    u8 write;
};

static bool operator==(ComponentId lhs, ComponentId rhs) { return lhs.Value == rhs.Value; }
static bool operator!=(ComponentId lhs, ComponentId rhs) { return lhs.Value == rhs.Value; }
static bool operator<(ComponentId lhs, ComponentId rhs) { return lhs.Value < rhs.Value; }

static bool operator==(ComponentType lhs, ComponentType rhs) { return lhs.id == rhs.id; }
static bool operator!=(ComponentType lhs, ComponentType rhs) { return lhs.id == rhs.id; }
static bool operator<(ComponentType lhs, ComponentType rhs) { return lhs.id < rhs.id; }

namespace ECS
{
    ComponentId RegisterType(Guid guid, i32 sizeOf);

    template<typename T>
    static ComponentId GetId()
    {
        static ComponentId id = RegisterType(TGuidOf<T>(), sizeof(T));
        return id;
    }
};

template<typename T>
static ComponentType CTypeOf(bool write)
{
    return { ECS::GetId<T>(), (u8)write };
}
