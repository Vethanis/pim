#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct pt_scene_s pt_scene_t;

#define kGiDirections 9

typedef struct giprobe_s
{
    // spherical gaussian amplitude in each direction
    float4 Values[kGiDirections];
} giprobe_t;

typedef struct gigrid_s
{
    float4 lo;
    float4 hi;
    float4 probesPerMeter;
    float4 stride;
    i32 size;
    i32 length;
    float rcpSize;
    giprobe_t* probes;
} gigrid_t;

gigrid_t* gigrid_get(void);

void gigrid_new(gigrid_t* grid, i32 size, box_t bounds);
void gigrid_del(gigrid_t* grid);

float4 VEC_CALL gigrid_sample(const gigrid_t* grid, float4 position, float4 direction);

void VEC_CALL gigrid_blend(gigrid_t* grid, float4 position, float4 direction, float4 radiance, float weight);

void gigrid_bake(gigrid_t* grid, const pt_scene_t* scene, float weight);

PIM_C_END
