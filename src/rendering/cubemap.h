#pragma once

#include "common/macro.h"
#include "math/types.h"

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

Cubemap* Cubemap_New(i32 size);
void Cubemap_Del(Cubemap* cm);

float4 VEC_CALL Cubemap_Read(const Cubemap* cm, float4 dir, float mip);
void VEC_CALL Cubemap_Write(Cubemap* cm, Cubeface face, int2 coord, float4 value);

void VEC_CALL Cubemap_WriteMip(Cubemap* cm, Cubeface face, int2 coord, i32 mip, float4 value);

float4 VEC_CALL Cubemap_CalcDir(i32 size, Cubeface face, int2 coord, float2 Xi);

float4 VEC_CALL Cubemap_FaceDir(Cubeface face);

void Cubemap_GenMips(Cubemap* cm);

struct task_s* Cubemap_Bake(
    Cubemap* cm,
    const struct pt_scene_s* scene,
    float4 origin,
    float sampleWeight);

PIM_C_END
