#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/texture.h"

PIM_C_BEGIN

typedef struct material_s
{
    float4 st;              // uv scale and translation
    u32 flatAlbedo;         // rgba8 srgb (albedo, alpha)
    u32 flatRome;           // rgba8 srgb (roughness, occlusion, metallic, emission)
    textureid_t albedo;     // rgba8 srgb (albedo, alpha)
    textureid_t rome;       // rgba8 srgb (roughness, occlusion, metallic, emission)
} material_t;

float4 VEC_CALL material_albedo(const material_t* mat, float2 uv);
float4 VEC_CALL material_rome(const material_t* mat, float2 uv);

PIM_C_END
