#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"

PIM_C_BEGIN

pim_inline float4 VEC_CALL AmbCube_Eval(AmbCube_t c, float4 dir)
{
    bool4 face = f4_gtvs(dir, 0.0f);
    dir = f4_mul(dir, dir);
    float4 v = f4_0;

    if (face.x)
        v = f4_add(v, f4_mulvs(c.xp, dir.x));
    else
        v = f4_add(v, f4_mulvs(c.xn, dir.x));

    if (face.y)
        v = f4_add(v, f4_mulvs(c.yp, dir.y));
    else
        v = f4_add(v, f4_mulvs(c.yn, dir.y));

    if (face.z)
        v = f4_add(v, f4_mulvs(c.zp, dir.z));
    else
        v = f4_add(v, f4_mulvs(c.zn, dir.z));

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
        c.xp = f4_lerpvs(c.xp, rad, dir.x);
    else
        c.xn = f4_lerpvs(c.xn, rad, dir.x);

    if (face.y)
        c.yp = f4_lerpvs(c.yp, rad, dir.y);
    else
        c.yn = f4_lerpvs(c.yn, rad, dir.y);

    if (face.z)
        c.zp = f4_lerpvs(c.zp, rad, dir.z);
    else
        c.zn = f4_lerpvs(c.zn, rad, dir.z);

    return c;
}

i32 AmbCube_Bake(
    const struct pt_scene_s* scene,
    AmbCube_t* pCube,
    float4 origin,
    i32 samples,
    i32 prevSampleCount,
    i32 bounces);

PIM_C_END
