#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "rendering/path_tracer.h"
#include "threading/task.h"

PIM_C_BEGIN

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
    float4* faces[Cubeface_COUNT];
} Cubemap;
static const u32 Cubemap_hash = 3773973750u;

typedef struct BCubemap_s
{
    i32 size;
    trace_img_t faces[Cubeface_COUNT];
} BCubemap;
static const u32 BCubemap_hash = 1293427894u;

void Cubemap_New(Cubemap* cm, i32 size);
void Cubemap_Del(Cubemap* cm);

void BCubemap_New(BCubemap* cm, i32 size);
void BCubemap_Del(BCubemap* cm);

void Cubemap_Cpy(const Cubemap* src, Cubemap* dst);

Cubeface VEC_CALL Cubemap_CalcUv(float4 dir, float2* uvOut);

pim_inline float VEC_CALL Cubemap_Rough2Mip(float roughness)
{
    return roughness * 6.0f;
}

pim_inline float VEC_CALL Cubemap_Mip2Rough(float mip)
{
    return mip / 6.0f;
}

float4 VEC_CALL Cubemap_Read(const Cubemap* cm, float4 dir, float mip);
void VEC_CALL Cubemap_Write(Cubemap* cm, Cubeface face, int2 coord, float4 value);

void VEC_CALL Cubemap_WriteMip(Cubemap* cm, Cubeface face, int2 coord, i32 mip, float4 value);

float4 VEC_CALL Cubemap_CalcDir(i32 size, Cubeface face, int2 coord, float2 Xi);

float4 VEC_CALL Cubemap_FaceDir(Cubeface face);

void Cubemap_GenMips(Cubemap* cm);

task_t* Cubemap_Bake(
    BCubemap* bakemap,
    const pt_scene_t* scene,
    float4 origin,
    float weight,
    i32 bounces);

void Cubemap_Denoise(BCubemap* src, Cubemap* dst);

void Cubemap_Prefilter(const Cubemap* src, Cubemap* dst, u32 sampleCount);

PIM_C_END
