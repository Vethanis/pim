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
    matflag_portal = 1 << 9,
} MatFlagBits;
typedef u32 MatFlag;

typedef struct Material_s
{
    TextureId albedo;     // rgba8 srgb (albedo, alpha)
    TextureId rome;       // rgba8 srgb (roughness, occlusion, metallic, emission)
    TextureId normal;     // rgba8 (tangent space xyz, packed as unorm)
    MatFlag flags;
    float ior;              // index of refraction
    float bumpiness;
} Material;

typedef struct DiskMaterial_s
{
    DiskTextureId albedo;
    DiskTextureId rome;
    DiskTextureId normal;
    MatFlag flags;
    float ior;
} DiskMaterial;

PIM_C_END
