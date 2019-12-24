#pragma once

#include "common/int_types.h"

enum ComponentId : u16
{
    ComponentId_Identifier,

    ComponentId_Immovable,
    ComponentId_Translation,
    ComponentId_Rotation,
    ComponentId_Scale,
    ComponentId_LocalToWorld,
    ComponentId_LocalToParent,
    ComponentId_Parent,
    ComponentId_Children,

    ComponentId_Drawable,
    ComponentId_RenderBounds,
    ComponentId_Camera,
    ComponentId_Light,
    ComponentId_ParticleEmitter,
    ComponentId_FogVolume,

    ComponentId_Collider,
    ComponentId_Rigidbody,

    ComponentId_SoundTag,
    ComponentId_SoundArea,
    ComponentId_SoundEmitter,
    ComponentId_SoundListener,

    ComponentId_COUNT
};
