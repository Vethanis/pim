#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "threading/task.h"

PIM_C_BEGIN

typedef struct material_s material_t;
typedef struct camera_s camera_t;
typedef struct prng_s prng_t;

typedef enum
{
    ptdist_sphere,
    ptdist_hemi,

} pt_dist_t;

typedef struct pt_scene_s
{
    // all meshes within the scene
    // xyz: vertex position
    //   w: 1
    float4* positions;
    // xyz: vertex normal
    float4* normals;
    //  xy: texture coordinate
    float2* uvs;
    // material indices
    i32* matIds;

    // surface description, indexed by normal.w
    material_t* materials;

    // full octree
    // child(p, i) = 8 * p + i + 1
    box_t* boxes; // aabb of node
    i32** trilists; // [N, f0v0, f1v0, f2v0, ..., fN-1v0]
    i32* pops; // object population in subtree

    i32 vertCount;
    i32 matCount;
    i32 lightCount;
    i32 nodeCount;
} pt_scene_t;

typedef struct pt_trace_s
{
    const pt_scene_t* scene;
    const camera_t* camera;
    float4* image;
    int2 imageSize;
    float sampleWeight;
    i32 bounces;
} pt_trace_t;

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    ray_t origin;
    float4* colors;
    float4* directions;
    pt_dist_t dist;
    i32 bounces;
} pt_raygen_t;

pt_scene_t* pt_scene_new(i32 maxDepth);
void pt_scene_del(pt_scene_t* scene);

float4 VEC_CALL pt_trace_ray(prng_t* rng, const pt_scene_t* scene, ray_t ray, i32 bounces);
task_t* pt_trace(pt_trace_t* traceDesc);
pt_raygen_t* pt_raygen(const pt_scene_t* scene, ray_t origin, pt_dist_t dist, i32 count, i32 bounces);

PIM_C_END
