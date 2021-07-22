#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/texture.h"

PIM_C_BEGIN

typedef enum
{
    MatFlag_Emissive = 1 << 0,      // enable emissive term
    MatFlag_Sky = 1 << 1,
    MatFlag_Water = 1 << 2,
    MatFlag_Slime = 1 << 3,
    MatFlag_Lava = 1 << 4,
    MatFlag_Refractive = 1 << 5,    // enable refraction term
    MatFlag_Warped = 1 << 6,        // uv animated
    MatFlag_Animated = 1 << 7,      // keyframe animated
    MatFlag_Underwater = 1 << 8,    // SURF_UNDERWATER
    MatFlag_Portal = 1 << 9,
    MatFlag_Diffuse = 1 << 10,      // enable diffuse term
    MatFlag_Specular = 1 << 11,     // enable specular term
    MatFlag_Volume = 1 << 12,       // enable volumetric term
} MatFlagBits;
typedef u32 MatFlag;

typedef struct Material_s
{
    float4 uMin;
    float4 uMax;
    TextureId albedo;       // rgba8 srgb (albedo, alpha)
    TextureId rome;         // rgba8 srgb (roughness, occlusion, metallic, emission)
    TextureId normal;       // rg16 (tangent space xy)
    TextureId density;      // r8 3D
    MatFlag flags;
    float ior;              // index of refraction
    float bumpiness;
    float g0;
    float g1;
    float gBlend;
    float absorption;
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
