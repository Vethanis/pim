#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/path_tracer.h"
#include "threading/task.h"

PIM_C_BEGIN

typedef struct SphereMap_s
{
    int2 size;
    float4* texels;
} SphereMap;

void SphereMap_New(SphereMap* map, int2 size);
void SphereMap_Del(SphereMap* map);

float2 VEC_CALL SphereMap_DirToUv(float4 dir);
float4 VEC_CALL SphereMap_UvToDir(float2 Xi);

float4 VEC_CALL SphereMap_Read(const SphereMap* map, float4 dir);
void VEC_CALL SphereMap_Write(SphereMap* map, float4 dir, float4 color);

void VEC_CALL SphereMap_Fit(SphereMap* map, float4 dir, float4 color, float weight);
task_t* SphereMap_Bake(
    const pt_scene_t* scene,
    trace_img_t img,
    float4 origin,
    float weight,
    i32 bounces);

PIM_C_END
