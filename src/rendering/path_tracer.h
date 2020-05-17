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

    // full octree
    // child(p, i) = 8 * p + i + 1
    box_t* boxes; // aabb of node
    i32** trilists; // [N, f0v0, f1v0, f2v0, ..., fN-1v0]
    i32** lightlists; // [N, L0, L1, L2, ...]
    i32* pops; // object population in subtree

    i32 vertCount;
    i32 matCount;
    i32 lightCount;
    i32 nodeCount;
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
pt_scene_t* pt_scene_new(i32 maxDepth);
void pt_scene_del(pt_scene_t* scene);

struct task_s* pt_trace(pt_trace_t* traceDesc);

float4 VEC_CALL pt_trace_frag(struct prng_s* rng, const pt_scene_t* scene, ray_t ray, i32 bounces);

PIM_C_END
