#pragma once

#include "pt_types_public.h"

PIM_C_BEGIN

typedef struct Cubemap_s Cubemap;
typedef struct Material_s Material;

typedef struct PtSurfHit_s
{
    float4 P;
    float4 M; // macro normal
    float4 N; // micro normal
    float4 albedo;
    float4 emission;
    float4 meanFreePath;
    float roughness;
    float occlusion;
    float metallic;
    float ior;
    u32 flags; // MatFlag
    PtHitType type;
} PtSurfHit;

typedef struct PtScatter_s
{
    float4 pos;
    float4 dir;
    float4 attenuation;
    float4 luminance;
    float pdf;
} PtScatter;

typedef struct PtLightSample_s
{
    float4 direction;
    float4 luminance;
    float4 wuvt;
    float pdf;
} PtLightSample;

typedef struct PtMediaDesc_s
{
    float4 constantColor;
    float4 noiseColor;
    float constantMfp;
    float noiseMfp;
    float4 constantMu;
    float4 noiseMu;
    float rcpMajorant;
    float absorption;
    i32 noiseOctaves;
    float noiseGain;
    float noiseLacunarity;
    float noiseFreq;
    float noiseScale;
    float noiseHeight;
    float noiseRange;
    float phaseDirA;
    float phaseDirB;
    float phaseBlend;
} PtMediaDesc;

typedef struct PtMedia_s
{
    float4 scattering;
    float4 extinction;
    float g0, g1, gBlend;
} PtMedia;

typedef struct RTCSceneTy* RTCScene;

typedef struct PtScene_s
{
    RTCScene rtcScene;

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
    // emissive index
    // [vertCount]
    i32* pim_noalias vertToEmit;

    // emissive triangle indices
    // [emissiveCount]
    i32* pim_noalias emitToVert;

    // grid of discrete light distributions
    Grid lightGrid;
    // [lightGrid.size]
    Dist1D* pim_noalias lightDists;

    // surface description, indexed by matIds
    // [matCount]
    Material* pim_noalias materials;

    Cubemap* pim_noalias sky;

    // array lengths
    i32 vertCount;
    i32 matCount;
    i32 emissiveCount;
    // parameters
    PtMediaDesc mediaDesc;
    u64 modtime;
} PtScene;

typedef struct PtContext_s
{
    PtSampler* pim_noalias sampler;
    PtScene* pim_noalias scene;
} PtContext;

PIM_C_END
