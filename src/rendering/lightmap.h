#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;

typedef struct lightmap_s
{
    float3* color;
    float3* denoised;
    float3* position;
    float3* normal;
    float* sampleCounts;
    i32 size;
} lightmap_t;

typedef struct lmpack_s
{
    i32 lmCount;
    i32 lmSize;
    lightmap_t* lightmaps;
} lmpack_t;

void lightmap_new(lightmap_t* lm, i32 size);
void lightmap_del(lightmap_t* lm);

lmpack_t* lmpack_get(void);
lmpack_t lmpack_pack(
    i32 atlasSize,
    float texelsPerUnit,
    float distThresh,
    float degThresh);
void lmpack_del(lmpack_t* pack);

void lmpack_bake(const pt_scene_t* scene, i32 bounces, float timeSlice);

void lmpack_denoise(void);

PIM_C_END
