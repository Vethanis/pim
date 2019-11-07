#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "components/entity.h"
#include "components/component.h"
#include "containers/array.h"
#include "components/ecs.h"

struct Translation
{
    float3 Value;
};
DeclComponent(Translation, )

struct Rotation
{
    quaternion Value;
};
DeclComponent(Rotation, )

struct Scale
{
    float3 Value;
};
DeclComponent(Scale, )

struct LocalToWorld
{
    float4x4 Value;
};
DeclComponent(LocalToWorld, )

struct Children
{
    Array<Entity> Value;

    inline void Drop()
    {
        for (Entity child : Value)
        {
            Ecs::Destroy(child);
        }
        Value.reset();
    }
};
DeclComponent(Children, ptr->Drop())
