#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "rendering/texture.h"

typedef struct material_s
{
    float4 st;              // uv scale and translation
    float4 lmst;            // lightmap uv scale and translation
    textureid_t albedo;     // rgba8 albedo, alpha
    textureid_t rome;       // rgba8 roughness, occlusion, metallic, emission
} material_t;

PIM_C_END
