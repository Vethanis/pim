#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/float4.h"
#include "math/float2.h"

typedef struct mesh_s
{
    float4* positions;
    float4* normals;
    float2* uvs;
    i32 length;
} mesh_t;

PIM_C_END
