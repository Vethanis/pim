#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct task_s task_t;
typedef struct pt_scene_s pt_scene_t;

typedef struct chartnode_s
{
    tri2d_t tri;
    float area;
    i32 drawableIndex;
    i32 vertIndex;
    i32 atlasIndex;
} chartnode_t;

typedef struct lightmap_s
{
    float4* texels;
    float* sampleCounts;
    i32 size;
} lightmap_t;

typedef struct lmpack_s
{
    i32 lmCount;
    lightmap_t* lightmaps;
    i32 nodeCount;
    chartnode_t* nodes;
} lmpack_t;

void lightmap_new(lightmap_t* lm, i32 size);
void lightmap_del(lightmap_t* lm);

lmpack_t* lmpack_get(void);
lmpack_t lmpack_pack(i32 atlasSize, float texelsPerUnit);
void lmpack_del(lmpack_t* pack);

task_t* lmpack_bake(const pt_scene_t* scene, i32 bounces);

PIM_C_END
