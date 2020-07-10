#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;

#define kGiDirections 9

typedef struct lightmap_s
{
    float4* pim_noalias probes;
    float* pim_noalias weights;
    float3* pim_noalias color;
    float3* pim_noalias denoised;
    float3* pim_noalias position;
    float3* pim_noalias normal;
    float* pim_noalias sampleCounts;
    i32 size;
} lightmap_t;

typedef struct lmpack_s
{
    i32 lmCount;
    i32 lmSize;
    lightmap_t* pim_noalias lightmaps;
    float4 axii[kGiDirections];
} lmpack_t;

typedef struct lm_uvs_s
{
    i32 length;
    float2* pim_noalias uvs;
    i32* pim_noalias indices;
} lm_uvs_t;

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

void lmpack_bake(const pt_scene_t* scene, float timeSlice);

void lmpack_denoise(void);

void lm_uvs_new(lm_uvs_t* uvs, i32 length);
void lm_uvs_del(lm_uvs_t* uvs);

PIM_C_END
