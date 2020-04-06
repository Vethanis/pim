#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/float4.h"

typedef struct ent_s
{
    i32 index;
    i32 version;
} ent_t;

typedef struct position_s
{
    float4 Value;
} position_t;

typedef struct rotation_s
{
    float4 Value;
} rotation_t;

typedef struct scale_s
{
    float4 Value;
} scale_t;

typedef enum
{
    CompId_Entity,
    CompId_Position,
    CompId_Rotation,
    CompId_Scale,

    CompId_COUNT
} compid_t;

PIM_C_END
