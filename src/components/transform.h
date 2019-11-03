#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "components/entity.h"
#include "containers/array.h"

struct Translation
{
    float3 Value;

    DeclComponent(Translation, )
};

struct Rotation
{
    quaternion Value;

    DeclComponent(Rotation, )
};

struct Scale
{
    float3 Value;

    DeclComponent(Scale, )
};

struct LocalToWorld
{
    float4x4 Value;

    DeclComponent(LocalToWorld, )
};

struct Children
{
    Array<Entity> Value;

    DeclComponent(Children, ptr->Value.reset())
};
