#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "common/random.h"

PIM_C_BEGIN

typedef struct Material_s Material;
typedef struct Camera_s Camera;
typedef struct Task_s Task;

typedef struct PtScene_s PtScene;

typedef struct PtSampler_s
{
    Prng rng;
    float Xi[10];
} PtSampler;

typedef enum
{
    PtHit_Nothing = 0,
    PtHit_Backface,
    PtHit_Triangle,

    PtHit_COUNT
} PtHitType;

typedef struct PtRayHit_s
{
    float4 wuvt;
    float4 normal;
    PtHitType type;
    i32 iVert;
    u32 flags;
} PtRayHit;

typedef struct PtDofInfo_s
{
    float aperture;
    float focalLength;
    i32 bladeCount;
    float bladeRot;
    float focalPlaneCurvature;
    float autoFocusSpeed;
    bool autoFocus;
} PtDofInfo;

typedef struct PtTrace_s
{
    PtScene* scene;
    float3* pim_noalias color;
    float3* pim_noalias albedo;
    float3* pim_noalias normal;
    float3* pim_noalias denoised;
    int2 imageSize;
    float sampleWeight;
    PtDofInfo dofinfo;
} PtTrace;

typedef struct PtResult_s
{
    float3 color;
    float3 albedo;
    float3 normal;
} PtResult;

typedef struct PtResults_s
{
    float4* colors;
    float4* directions;
} PtResults;

PIM_C_END
