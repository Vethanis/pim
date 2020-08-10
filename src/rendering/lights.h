#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct dir_light_s
{
    float4 dir;
    float4 rad;
} dir_light_t;

typedef struct pt_light_s
{
    float4 pos;
    float4 rad;
} pt_light_t;

typedef struct lights_s
{
    dir_light_t* dirLights;
    pt_light_t* ptLights;
    i32 dirCount;
    i32 ptCount;
} lights_t;

lights_t* lights_get(void);

void lights_clear(void);

i32 lights_add_pt(pt_light_t pt);
i32 lights_add_dir(dir_light_t dir);

void lights_rm_pt(i32 i);
void lights_rm_dir(i32 i);

void lights_set_pt(i32 i, pt_light_t pt);
void lights_set_dir(i32 i, dir_light_t dir);

pt_light_t lights_get_pt(i32 i);
dir_light_t lights_get_dir(i32 i);

i32 lights_pt_count(void);
i32 lights_dir_count(void);

PIM_C_END
