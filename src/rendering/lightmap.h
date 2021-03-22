#pragma once

#include "common/macro.h"
#include "common/dbytes.h"
#include "math/types.h"
#include "common/guid.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

#define kLmPackVersion      2
#define kGiDirections       5

static const float4 kGiAxii[kGiDirections] =
{
    { 0.000000f, 0.000000f, 1.000000f, 4.999773f },
    { 0.577350f, 0.577350f, 0.577350f, 4.999773f },
    { -0.577350f, 0.577350f, 0.577350f, 4.999773f },
    { 0.577350f, -0.577350f, 0.577350f, 4.999773f },
    { -0.577350f, -0.577350f, 0.577350f, 4.999773f },
};

typedef struct Task_s Task;
typedef struct PtScene_s PtScene;
typedef struct Crate_s Crate;

typedef struct Lightmap_s
{
    float4* pim_noalias probes[kGiDirections];
    float3* pim_noalias position;
    float3* pim_noalias normal;
    float* pim_noalias sampleCounts;
    i32 size;
    vkrTextureId slot;
} Lightmap;

typedef struct LmPack_s
{
    float4 axii[kGiDirections];
    Lightmap* pim_noalias lightmaps;
    i32 lmCount;
    i32 lmSize;
    float texelsPerMeter;
} LmPack;

typedef struct DiskLmPack_s
{
    i32 version;
    i32 directions;
    i32 lmCount;
    i32 lmSize;
    i32 bytesPerLightmap;
    float texelsPerMeter;
} DiskLmPack;

void Lightmap_New(Lightmap* lm, i32 size);
void Lightmap_Del(Lightmap* lm);
// upload changes to the GPU copy
void Lightmap_Upload(Lightmap* lm);

LmPack* LmPack_Get(void);
LmPack LmPack_Pack(
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh);
void LmPack_Del(LmPack* pack);

void LmPack_Bake(PtScene* scene, float timeSlice, i32 spp);

bool LmPack_Save(Crate* crate, const LmPack* src);
bool LmPack_Load(Crate* crate, LmPack* dst);

PIM_C_END
