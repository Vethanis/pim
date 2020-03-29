#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

typedef struct ent_s
{
    int32_t index;
    int32_t version;
} ent_t;

typedef struct position_s
{
    float Value[4];
} position_t;

typedef struct rotation_s
{
    float Value[4];
} rotation_t;

typedef struct scale_s
{
    float Value[4];
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
