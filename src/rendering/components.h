#pragma once

#include "math/vec_types.h"
#include "geometry/types.h"
#include "common/type_guid.h"

struct MeshId
{
    i32 version;
    i32 index;
};

struct MaterialId
{
    i32 version;
    i32 index;
};

enum class LightType : u8
{
    Directional,
    Point,
    Spot,

    COUNT
};

enum class BoundsType : u8
{
    Sphere,
    Box,

    COUNT
};

// ----------------------------------------------------------------------------

struct Drawable
{
    MeshId mesh;
    MaterialId material;
};
DECL_TGUID(Drawable)

struct RenderBounds
{
    BoundsType type;
    float3 extents;
};
DECL_TGUID(RenderBounds)

struct Camera
{
    float fieldOfView;
    float nearPlane;
    float farPlane;
};
DECL_TGUID(Camera)

struct Light
{
    LightType type;
    float angle;
    float intensity;
    float3 color;
};
DECL_TGUID(Light)

struct ParticleEmitter
{

};
DECL_TGUID(ParticleEmitter)

struct FogVolume
{
    float density;
    float3 color;
};
DECL_TGUID(FogVolume)
