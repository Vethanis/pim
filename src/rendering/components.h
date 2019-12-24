#pragma once

#include "components/component.h"
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
    static constexpr ComponentId Id = ComponentId_Drawable;
};

struct RenderBounds
{
    BoundsType type;
    float3 extents;
    static constexpr ComponentId Id = ComponentId_RenderBounds;
};

struct Camera
{
    float fieldOfView;
    float nearPlane;
    float farPlane;
    static constexpr ComponentId Id = ComponentId_Camera;
};

struct Light
{
    LightType type;
    float angle;
    float intensity;
    float3 color;
    static constexpr ComponentId Id = ComponentId_Light;
};

struct ParticleEmitter
{
    static constexpr ComponentId Id = ComponentId_ParticleEmitter;
};

struct FogVolume
{
    float density;
    float3 color;
    static constexpr ComponentId Id = ComponentId_FogVolume;
};

#define COMPONENTS_RENDERING(XX) \
    XX(Drawable) \
    XX(RenderBounds) \
    XX(Camera) \
    XX(Light) \
    XX(ParticleEmitter) \
    XX(FogVolume)
