#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/random.h"

PIM_C_BEGIN

typedef struct material_s material_t;
typedef struct camera_s camera_t;

typedef struct pt_scene_s pt_scene_t;

typedef enum
{
    hit_nothing = 0,
    hit_backface,
    hit_triangle,
    hit_sky,

    hit_COUNT
} hittype_t;

typedef struct rayhit_s
{
    float4 wuvt;
    float4 normal;
    hittype_t type;
    i32 index;
} rayhit_t;

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

void pt_sys_init(void);
void pt_sys_update(void);
void pt_sys_shutdown(void);

pt_scene_t* pt_scene_new(void);
void pt_scene_del(pt_scene_t* scene);
void pt_scene_gui(pt_scene_t* scene);

rayhit_t VEC_CALL pt_intersect(const pt_scene_t* scene, ray_t ray, float tNear, float tFar);

pt_result_t VEC_CALL pt_trace_ray(
    prng_t* rng,
    const pt_scene_t* scene,
    ray_t ray);

void pt_trace(pt_trace_t* traceDesc);

pt_results_t pt_raygen(
    pt_scene_t* scene,
    float4 origin,
    i32 count);

PIM_C_END
