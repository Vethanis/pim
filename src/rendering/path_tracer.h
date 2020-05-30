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

typedef struct trace_img_s
{
    int2 size;
    float3* colors;
    float3* albedos;
    float3* normals;
} trace_img_t;

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
    trace_img_t img;
    float sampleWeight;
    i32 bounces;
} pt_trace_t;

typedef struct pt_result_s
{
    float3 color;
    float3 albedo;
    float3 normal;
} pt_result_t;

typedef struct pt_raygen_s
{
    task_t task;
    const pt_scene_t* scene;
    ray_t origin;
    float3* colors;
    float3* albedos;
    float3* normals;
    float4* directions;
    pt_dist_t dist;
    i32 bounces;
} pt_raygen_t;

void trace_img_new(trace_img_t* img, int2 size);
void trace_img_del(trace_img_t* img);

pt_scene_t* pt_scene_new(i32 maxDepth);
void pt_scene_del(pt_scene_t* scene);

pt_result_t VEC_CALL pt_trace_ray(prng_t* rng, const pt_scene_t* scene, ray_t ray, i32 bounces);
task_t* pt_trace(pt_trace_t* traceDesc);
pt_raygen_t* pt_raygen(const pt_scene_t* scene, ray_t origin, pt_dist_t dist, i32 count, i32 bounces);

PIM_C_END
