#pragma once

#include "common/macro.h"
#include "common/dbytes.h"
#include "math/types.h"
#include "common/guid.h"

PIM_C_BEGIN

#define kLightmapVersion    1
#define kLmPackVersion      1
#define kGiDirections       5

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;

typedef struct lightmap_s
{
    float4* pim_noalias probes[kGiDirections];
    float3* pim_noalias position;
    float3* pim_noalias normal;
    float* pim_noalias sampleCounts;
    i32 size;
} lightmap_t;

typedef struct dlightmap_s
{
    i32 version;
    i32 directions;
    i32 size;
    dbytes_t probes;
    dbytes_t position;
    dbytes_t normal;
    dbytes_t sampleCounts;
} dlightmap_t;

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
    float4 axii[kGiDirections];
    float texelsPerMeter;
} dlmpack_t;

typedef struct lm_uvs_s
{
    i32 length;
    float2* pim_noalias uvs;
    i32* pim_noalias indices;
} lm_uvs_t;

typedef struct dlm_uvs_s
{
    i32 length;
    dbytes_t uvs;
    dbytes_t indices;
} dlm_uvs_t;

void lightmap_new(lightmap_t* lm, i32 size);
void lightmap_del(lightmap_t* lm);

lmpack_t* lmpack_get(void);
lmpack_t lmpack_pack(
    const pt_scene_t* scene,
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh);
void lmpack_del(lmpack_t* pack);

void lmpack_bake(pt_scene_t* scene, float timeSlice);

bool lmpack_save(const lmpack_t* src, guid_t name);
bool lmpack_load(lmpack_t* dst, guid_t name);

void lm_uvs_new(lm_uvs_t* uvs, i32 length);
void lm_uvs_del(lm_uvs_t* uvs);

PIM_C_END
