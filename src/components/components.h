#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

PIM_C_BEGIN

typedef struct ent_s
{
    i32 index;
    i32 version;
} ent_t;
static const u32 ent_t_hash = 3006851633u;

typedef struct tag_s
{
    u32 Value;
} tag_t;
static const u32 tag_t_hash = 2660451440u;

typedef struct translation_s
{
    float4 Value;
} translation_t;
static const u32 translation_t_hash = 2961877959u;

typedef struct rotation_s
{
    quat Value;
} rotation_t;
static const u32 rotation_t_hash = 2378169660u;

typedef struct scale_s
{
    float4 Value;
} scale_t;
static const u32 scale_t_hash = 1198251898u;

typedef struct localtoworld_s
{
    float4x4 Value;         // transforms entity from local space to world space
} localtoworld_t;
static const u32 localtoworld_t_hash = 3458620036u;

typedef struct drawable_s
{
    meshid_t mesh;          // local space immutable mesh
    material_t material;    // textures and surface properties
    meshid_t tmpmesh;       // world space temporary mesh
    u64 tilemask;           // bitmask of tile visibility
} drawable_t;
static const u32 drawable_t_hash = 4217122476u;

typedef struct bounds_s
{
    sphere_t Value;
} bounds_t;
static const u32 bounds_t_hash = 3603783155u;

typedef struct radiance_s
{
    float4 value;
} radiance_t;
static const u32 radiance_t_hash = 3387658039u;

typedef enum
{
    LightType_Directional = 0,
    LightType_Point,

    LightType_COUNT
} LightType;

typedef enum
{
    CompId_Tag,
    CompId_Translation,
    CompId_Rotation,
    CompId_Scale,
    CompId_LocalToWorld,
    CompId_Drawable,
    CompId_Bounds,

    CompId_COUNT
} compid_t;

PIM_C_END
