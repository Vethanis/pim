#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/texture.h"

PIM_C_BEGIN

typedef enum
{
    matflag_emissive = 1 << 0,
    matflag_sky = 1 << 1,
    matflag_lava = 1 << 2,
    matflag_refractive = 1 << 3,
    matflag_animated = 1 << 4,
} matflags_t;

typedef struct material_s
{
    float4 st;              // uv scale and translation
    u32 flatAlbedo;         // rgba8 srgb (albedo, alpha)
    u32 flatRome;           // rgba8 srgb (roughness, occlusion, metallic, emission)
    textureid_t albedo;     // rgba8 srgb (albedo, alpha)
    textureid_t rome;       // rgba8 srgb (roughness, occlusion, metallic, emission)
    textureid_t normal;     // rgba8 (tangent space xyz, packed as unorm)
    u32 flags;
} material_t;

PIM_C_END
