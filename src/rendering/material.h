#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "rendering/texture.h"

typedef struct material_s
{
    float4 st;              // uv scale and translation
    float4 lmst;            // lightmap uv scale and translation
    textureid_t albedo;     // rgba8 srgb albedo, alpha
    textureid_t rome;       // rgba8 srgb roughness, occlusion, metallic, emission
    textureid_t lightmap;   // rgbm8 diffuse light (m = scale)
} material_t;

PIM_C_END
