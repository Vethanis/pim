#pragma once

#include "math/vec_types.h"
#include "math/mat_types.h"
#include "components/entity.h"

struct Translation
{
    float3 Value;
};

struct Rotation
{
    quaternion Value;
};

struct Scale
{
    float3 Value;
};

struct ModelMatrix
{
    float4x4 Value;
};

struct Child
{
    Entity Value;
};
