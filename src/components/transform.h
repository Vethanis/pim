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

struct ModelMatrix
{
    float4x4 Value;
};
DeclComponent(ModelMatrix, )

struct Child
{
    Entity Value;
};
DeclComponent(Child, )
