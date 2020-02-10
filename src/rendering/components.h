#pragma once

#include "math/vec_types.h"
#include "geometry/types.h"

struct MeshId
{
    u16 version;
    u16 index;
};

struct MaterialId
{
    u16 version;
    u16 index;
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
    Capsule,

    COUNT
};

// ----------------------------------------------------------------------------

struct Drawable
{
    MeshId mesh;
    MaterialId material;
};

struct RenderBounds
{
    BoundsType type;
    float3 extents;
};

struct Camera
{
    float fieldOfView;
    float nearPlane;
    float farPlane;
};

struct Light
{
    LightType type;
    float angle;
    float intensity;
    float3 color;
};

struct ParticleEmitter
{

};

struct FogVolume
{
    float density;
    float3 color;
};
