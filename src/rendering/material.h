#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/texture.h"

PIM_C_BEGIN

typedef enum
{
    matflag_emissive = 1 << 0,
    matflag_sky = 1 << 1,
    matflag_water = 1 << 2,
    matflag_slime = 1 << 3,
    matflag_lava = 1 << 4,
    matflag_refractive = 1 << 5,
    matflag_warped = 1 << 6,        // uv animated
    matflag_animated = 1 << 7,      // keyframe animated
    matflag_underwater = 1 << 8,    // SURF_UNDERWATER
} matflagbits_t;
typedef u32 matflag_t;

typedef struct material_s
{
    float4 flatAlbedo;      // albedo multiplier
    float4 flatRome;        // rome multiplier
    textureid_t albedo;     // rgba8 srgb (albedo, alpha)
    textureid_t rome;       // rgba8 srgb (roughness, occlusion, metallic, emission)
    textureid_t normal;     // rgba8 (tangent space xyz, packed as unorm)
    matflag_t flags;
    float ior;              // index of refraction
} material_t;

typedef struct dmaterial_s
{
    dtextureid_t albedo;
    dtextureid_t rome;
    dtextureid_t normal;
    float4 flatAlbedo;
    float4 flatRome;
    matflag_t flags;
    float ior;
} dmaterial_t;

PIM_C_END
