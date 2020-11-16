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
    { 1.000000f, 0.000000f, 0.000000f, 3.210700f },
    { 0.267616f, 0.823639f, 0.500000f, 3.210700f },
    { -0.783327f, 0.569121f, 0.250000f, 3.210700f },
    { -0.535114f, -0.388783f, 0.750000f, 3.210700f },
    { 0.306594f, -0.943597f, 0.125000f, 3.210700f },
};

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;
typedef struct crate_s crate_t;

typedef struct lightmap_s
{
    vkrTextureId slots[kGiDirections];
    float4* pim_noalias probes[kGiDirections];
    float3* pim_noalias position;
    float3* pim_noalias normal;
    float* pim_noalias sampleCounts;
    i32 size;
} lightmap_t;

typedef struct lmpack_s
{
    float4 axii[kGiDirections];
    lightmap_t* pim_noalias lightmaps;
    i32 lmCount;
    i32 lmSize;
    float texelsPerMeter;
} lmpack_t;

typedef struct dlmpack_s
{
    i32 version;
    i32 directions;
    i32 lmCount;
    i32 lmSize;
    i32 bytesPerLightmap;
    float texelsPerMeter;
} dlmpack_t;

void lightmap_new(lightmap_t* lm, i32 size);
void lightmap_del(lightmap_t* lm);
// upload changes to the GPU copy
void lightmap_upload(lightmap_t* lm);

lmpack_t* lmpack_get(void);
lmpack_t lmpack_pack(
    const pt_scene_t* scene,
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh);
void lmpack_del(lmpack_t* pack);

void lmpack_bake(pt_scene_t* scene, float timeSlice, i32 spp);

bool lmpack_save(crate_t* crate, const lmpack_t* src);
bool lmpack_load(crate_t* crate, lmpack_t* dst);

PIM_C_END
