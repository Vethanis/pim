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
    void* rtcScene;

    // all geometry within the scene
    // xyz: vertex position
    //   w: 1
    // [vertCount]
    float4* pim_noalias positions;
    // xyz: vertex normal
    // [vertCount]
    float4* pim_noalias normals;
    //  xy: texture coordinate
    // [vertCount]
    float2* pim_noalias uvs;
    // material indices
    // [vertCount]
    i32* pim_noalias matIds;

    // emissive triangle indices
    // [emissiveCount]
    i32* pim_noalias emissives;
    // emissive texel chance of triangle
    // [emissiveCount]
    float* pim_noalias emPdfs;

    // surface description, indexed by matIds
    // [matCount]
    material_t* pim_noalias materials;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    i32 nodeCount;
    // hyperparameters
    i32 rejectionSamples;
    float rejectionThreshold;
    float3 sunDirection;
    float sunIntensity;
} pt_scene_t;

typedef struct pt_trace_s
{
    pt_scene_t* scene;
    const camera_t* camera;
    float3* color;
    float3* albedo;
    float3* normal;
    int2 imageSize;
    float sampleWeight;
} pt_trace_t;

typedef struct pt_result_s
{
    float3 color;
    float3 albedo;
    float3 normal;
} pt_result_t;

typedef struct pt_results_s
{
    float4* colors;
    float4* directions;
} pt_results_t;

pt_scene_t* pt_scene_new(i32 maxDepth, i32 rejectionSamples, float rejectionThreshold);
void pt_scene_del(pt_scene_t* scene);

pt_result_t VEC_CALL pt_trace_ray(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray);

void pt_trace(pt_trace_t* traceDesc);

pt_results_t pt_raygen(
    pt_scene_t* scene,
    ray_t origin,
    pt_dist_t dist,
    i32 count);

PIM_C_END
