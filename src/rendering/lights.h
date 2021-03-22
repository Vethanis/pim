#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

// TODO: Make these regular entities

typedef struct DirLight_s
{
    float4 dir;
    float4 rad;
} DirLight;

typedef struct PtLight_s
{
    float4 pos;
    float4 rad;
} PtLight;

typedef struct Lights_s
{
    DirLight* dirLights;
    PtLight* ptLights;
    i32 dirCount;
    i32 ptCount;
} Lights;

Lights* Lights_Get(void);

void Lights_Clear(void);

i32 Lights_AddPt(PtLight pt);
i32 Lights_AddDir(DirLight dir);

void Lights_RmPt(i32 i);
void Lights_RmDir(i32 i);

void Lights_SetPt(i32 i, PtLight pt);
void Lights_SetDir(i32 i, DirLight dir);

PtLight Lights_GetPt(i32 i);
DirLight Lights_GetDir(i32 i);

i32 Lights_PtCount(void);
i32 Lights_DirCount(void);

PIM_C_END
