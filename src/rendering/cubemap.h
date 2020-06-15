#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

#define CUBEMAP_DEFAULT_SIZE    64      // pow2(CUBEMAP_MAX_MIP)
#define CUBEMAP_MAX_MIP         6.0f    // log2(CUBEMAP_DEFAULT_SIZE)

typedef struct pt_scene_s pt_scene_t;

typedef enum
{
    Cubeface_XP,
    Cubeface_XM,
    Cubeface_YP,
    Cubeface_YM,
    Cubeface_ZP,
    Cubeface_ZM,

    Cubeface_COUNT
} Cubeface;

typedef struct Cubemap_s
{
    i32 size;
    i32 mipCount;
    float3* color[Cubeface_COUNT];
    float3* albedo[Cubeface_COUNT];
    float3* normal[Cubeface_COUNT];
    float3* denoised[Cubeface_COUNT];
    float4* convolved[Cubeface_COUNT];
} Cubemap;

typedef struct Cubemaps_s
{
    i32 count;
    u32* names;
    Cubemap* cubemaps;
    sphere_t* bounds;
} Cubemaps_t;

Cubemaps_t* Cubemaps_Get(void);

i32 Cubemaps_Add(u32 name, i32 size, sphere_t bounds);
bool Cubemaps_Rm(u32 name);
i32 Cubemaps_Find(u32 name);

void Cubemap_New(Cubemap* cm, i32 size);
void Cubemap_Del(Cubemap* cm);

Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut);

// note: input is roughness, not alpha (aka roughness^2)
pim_inline float VEC_CALL RoughnessToMip(float roughness)
{
    return roughness * CUBEMAP_MAX_MIP;
}

// note: output is roughness, not alpha (aka roughness^2)
pim_inline float VEC_CALL MipToRoughness(float mip)
{
    return mip / CUBEMAP_MAX_MIP;
}

float3 VEC_CALL Cubemap_ReadColor(const Cubemap* cm, float4 dir);
float3 VEC_CALL Cubemap_ReadAlbedo(const Cubemap* cm, float4 dir);
float3 VEC_CALL Cubemap_ReadNormal(const Cubemap* cm, float4 dir);
float3 VEC_CALL Cubemap_ReadDenoised(const Cubemap* cm, float4 dir);
float4 VEC_CALL Cubemap_ReadConvolved(const Cubemap* cm, float4 dir, float mip);

void VEC_CALL Cubemap_Write(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    float4 value);

void VEC_CALL Cubemap_WriteMip(
    Cubemap* cm,
    Cubeface face,
    int2 coord,
    i32 mip,
    float4 value);

float4 VEC_CALL Cubemap_CalcDir(
    i32 size,
    Cubeface face,
    int2 coord,
    float2 Xi);

void Cubemap_Bake(
    Cubemap* cm,
    const pt_scene_t* scene,
    float4 origin,
    float weight,
    i32 bounces);

void Cubemap_Denoise(Cubemap* cm);

void Cubemap_Convolve(
    Cubemap* cm,
    u32 sampleCount,
    float weight);

PIM_C_END
