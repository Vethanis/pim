#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef struct pt_scene_s pt_scene_t;

#define kGiDirections 5

typedef struct giprobe_s
{
    float4 Values[kGiDirections];
} giprobe_t;

typedef struct giprobeweights_s
{
    float Values[kGiDirections];
} giprobeweights_t;

typedef struct gigrid_s
{
    float4 lo;
    float4 hi;
    float4 rcpRange;
    giprobe_t* probes;
    giprobeweights_t* weights;
    float4* positions;
    int3 size;
    float4 axii[kGiDirections];
} gigrid_t;

gigrid_t* gigrid_get(void);

void gigrid_new(gigrid_t* grid, float probesPerMeter, box_t bounds);
void gigrid_del(gigrid_t* grid);

giprobe_t VEC_CALL gigrid_sample(
    const gigrid_t* grid,
    float4 position,
    float4 direction);

void gigrid_bake(gigrid_t* grid, const pt_scene_t* scene, i32 iSample, i32 samplesPerProbe);

PIM_C_END
