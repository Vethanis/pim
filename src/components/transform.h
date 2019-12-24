#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "components/entity.h"
#include "components/component.h"

struct Immovable
{
    static constexpr ComponentId Id = ComponentId_Immovable;
};

struct Translation
{
    float3 Value;
    static constexpr ComponentId Id = ComponentId_Translation;
};

struct Rotation
{
    quaternion Value;
    static constexpr ComponentId Id = ComponentId_Rotation;
};

struct Scale
{
    float3 Value;
    static constexpr ComponentId Id = ComponentId_Scale;
};

struct LocalToWorld
{
    float4x4 Value;
    static constexpr ComponentId Id = ComponentId_LocalToWorld;
};

struct LocalToParent
{
    float4x4 Value;
    static constexpr ComponentId Id = ComponentId_LocalToParent;
};

struct Children
{
    Array<Entity> Values;
    static constexpr ComponentId Id = ComponentId_Children;
};

struct Parent
{
    Entity Value;
    static constexpr ComponentId Id = ComponentId_Parent;
};

#define COMPONENTS_TRANSFORM(XX) \
    XX(Immovable) \
    XX(Translation) \
    XX(Rotation) \
    XX(Scale) \
    XX(LocalToWorld) \
    XX(LocalToParent) \
    XX(Parent) \
    XX(Children)
