#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"

PIM_C_BEGIN

typedef struct pt_scene_s pt_scene_t;

pim_inline float4 VEC_CALL AmbCube_Eval(AmbCube_t c, float4 dir)
{
    bool4 face = f4_gtvs(dir, 0.0f);
    dir = f4_mul(dir, dir);
    float4 v = f4_0;

    if (face.x)
        v = f4_add(v, f4_mulvs(c.Values[0], dir.x));
    else
        v = f4_add(v, f4_mulvs(c.Values[1], dir.x));

    if (face.y)
        v = f4_add(v, f4_mulvs(c.Values[2], dir.y));
    else
        v = f4_add(v, f4_mulvs(c.Values[3], dir.y));

    if (face.z)
        v = f4_add(v, f4_mulvs(c.Values[4], dir.z));
    else
        v = f4_add(v, f4_mulvs(c.Values[5], dir.z));

    return v;
}

pim_inline float4 VEC_CALL AmbCube_Irradiance(AmbCube_t c, float4 dir)
{
    return f4_mulvs(AmbCube_Eval(c, dir), kTau);
}

pim_inline AmbCube_t VEC_CALL AmbCube_Fit(AmbCube_t c, float weight, float4 dir, float4 rad)
{
    bool4 face = f4_gtvs(dir, 0.0f);
    dir = f4_mul(dir, dir);
    dir = f4_mulvs(dir, weight);

    if (face.x)
        c.Values[0] = f4_lerpvs(c.Values[0], rad, dir.x);
    else
        c.Values[1] = f4_lerpvs(c.Values[1], rad, dir.x);

    if (face.y)
        c.Values[2] = f4_lerpvs(c.Values[2], rad, dir.y);
    else
        c.Values[3] = f4_lerpvs(c.Values[3], rad, dir.y);

    if (face.z)
        c.Values[4] = f4_lerpvs(c.Values[4], rad, dir.z);
    else
        c.Values[5] = f4_lerpvs(c.Values[5], rad, dir.z);

    return c;
}

i32 AmbCube_Bake(
    pt_scene_t* scene,
    AmbCube_t* pCube,
    float4 origin,
    i32 samples,
    i32 prevSampleCount);

PIM_C_END
