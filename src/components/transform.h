#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "common/typeinfo.h"
#include "components/entity.h"
#include "containers/array.h"

struct TranslationComponent
{
    float3 Value;
};
Reflect(TranslationComponent);

struct RotationComponent
{
    quaternion Value;
};
Reflect(RotationComponent);

struct ScaleComponent
{
    float3 Value;
};
Reflect(ScaleComponent);

struct LocalToWorld
{
    float4x4 Value;
};
Reflect(LocalToWorld);

struct ChildrenComponent
{
    Array<Entity> Value;
};
Reflect(ChildrenComponent);
