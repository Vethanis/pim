#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "common/type_guid.h"

struct Translation
{
    float3 Value;
};
DECL_TGUID(Translation)

struct Rotation
{
    quaternion Value;
};
DECL_TGUID(Rotation)

struct Scale
{
    float3 Value;
};
DECL_TGUID(Scale)

struct LocalToWorld
{
    float4x4 Value;
};
DECL_TGUID(LocalToWorld)
