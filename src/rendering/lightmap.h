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

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;
typedef struct crate_s crate_t;

typedef struct lightmap_s
{
    float4* pim_noalias probes[kGiDirections];
    float3* pim_noalias position;
    float3* pim_noalias normal;
    float* pim_noalias sampleCounts;
    i32 size;
    vkrTextureId slot;
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
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh);
void lmpack_del(lmpack_t* pack);

void lmpack_bake(pt_scene_t* scene, float timeSlice, i32 spp);

bool lmpack_save(crate_t* crate, const lmpack_t* src);
bool lmpack_load(crate_t* crate, lmpack_t* dst);

PIM_C_END
