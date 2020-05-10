#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct pt_scene_s
{
    // all meshes within the scene
    // xyz: vertex position
    //   w: 1
    float4* positions;
    // xyz: vertex normal
    //   w: material index
    float4* normals;
    //  xy: texture coordinate
    float2* uvs;

    // surface description, indexed by normal.w
    struct material_s* materials;

    i32 vertCount;
    i32 matCount;
} pt_scene_t;

typedef struct pt_trace_s
{
    const pt_scene_t* scene;
    const struct camera_s* camera;
    float4* dstImage;
    int2 imageSize;
    float sampleWeight;
    i32 bounces;
} pt_trace_t;

// creates a scene from the current contents of the ECS
pt_scene_t* pt_scene_new(void);
void pt_scene_del(pt_scene_t* scene);

struct task_s* pt_trace(pt_trace_t* traceDesc);

PIM_C_END
