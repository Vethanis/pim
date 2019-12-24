#pragma once

#include "components/component.h"
#include "components/entity.h"
#include "common/guid.h"
#include "common/hashstring.h"
#include "math/vec_types.h"

struct SoundTag
{
    HashString tag;

    static constexpr ComponentId Id = ComponentId_SoundTag;
};

struct SoundArea
{
    float3 center;
    float3 extents;

    static constexpr ComponentId Id = ComponentId_SoundArea;
};

struct SoundEmitter
{
    Guid asset;
    float time;
    float volume;
    Entity source;
    bool loop;

    static constexpr ComponentId Id = ComponentId_SoundEmitter;
};

struct SoundListener
{
    float volume;

    static constexpr ComponentId Id = ComponentId_SoundListener;
};

#define COMPONENTS_AUDIO(XX) \
    XX(SoundTag) \
    XX(SoundArea) \
    XX(SoundEmitter) \
    XX(SoundListener)
