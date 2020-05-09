#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"

typedef struct pt_trace_s
{
    const struct pt_scene_s* scene;
    const struct camera_s* camera;
    float4* dstImage;
    int2 imageSize;
    i32 samplesPerPixel;
    i32 bounces;
} pt_trace_t;

// creates a scene from the current contents of the ECS
// if tags != 0, entities will be filtered to those with at least one matching tag
struct pt_scene_s* pt_scene_new(void);
void pt_scene_del(struct pt_scene_s* scene);

struct task_s* pt_trace(pt_trace_t* traceDesc);

PIM_C_END
