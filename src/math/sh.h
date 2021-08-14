#pragma once

#include "math/types.h"
#include "math/float3_funcs.h"

PIM_C_BEGIN

pim_inline float VEC_CALL SH4_dotss(SH4s a, SH4s b)
{
    float y = 0.0f;
    y += a.v[0] * b.v[0];
    y += a.v[1] * b.v[1];
    y += a.v[2] * b.v[2];
    y += a.v[3] * b.v[3];
    return y;
}

pim_inline float3 VEC_CALL SH4_dotvs(SH4v a, SH4s b)
{
    float3 y = f3_0;
    y = f3_add(y, f3_mulvs(a.v[0], b.v[0]));
    y = f3_add(y, f3_mulvs(a.v[1], b.v[1]));
    y = f3_add(y, f3_mulvs(a.v[2], b.v[2]));
    y = f3_add(y, f3_mulvs(a.v[3], b.v[3]));
    return y;
}

pim_inline float3 VEC_CALL SH4_dotsv(SH4s a, SH4v b)
{
    return SH4_dotvs(b, a);
}

pim_inline float3 VEC_CALL SH4_dotvv(SH4v a, SH4v b)
{
    float3 y = f3_0;
    y = f3_add(y, f3_mul(a.v[0], b.v[0]));
    y = f3_add(y, f3_mul(a.v[1], b.v[1]));
    y = f3_add(y, f3_mul(a.v[2], b.v[2]));
    y = f3_add(y, f3_mul(a.v[3], b.v[3]));
    return y;
}

pim_inline SH4s VEC_CALL SH4s_proj(
    float3 direction,
    float amplitude,
    float b0,
    float b1)
{
    SH4s y;

    b0 *= amplitude;
    b1 *= amplitude;

    y.v[0] = 0.282095f * b0;

    y.v[1] = -0.488603f * direction.x * b1;
    y.v[2] = 0.488603f * direction.y * b1;
    y.v[3] = -0.488603f * direction.z * b1;

    return y;
}

pim_inline SH4v VEC_CALL SH4v_proj(
    float3 direction,
    float3 amplitude,
    float b0,
    float b1)
{
    SH4v y;

    float3 ab0 = f3_mulvs(amplitude, b0);
    float3 ab1 = f3_mulvs(amplitude, b1);

    y.v[0] = f3_mulvs(ab0, 0.282095f);

    y.v[1] = f3_mulvs(ab1, -0.488603f * direction.x);
    y.v[2] = f3_mulvs(ab1, 0.488603f * direction.y);
    y.v[3] = f3_mulvs(ab1, -0.488603f * direction.z);

    return y;
}

pim_inline float VEC_CALL SH4s_eval(SH4s x, float3 direction)
{
    return SH4_dotss(x, SH4s_proj(direction, 1.0f, 1.0f, 1.0f));
}

pim_inline float3 VEC_CALL SH4v_eval(SH4v x, float3 direction)
{
    return SH4_dotvs(x, SH4s_proj(direction, 1.0f, 1.0f, 1.0f));
}

pim_inline float VEC_CALL SH4s_irradiance(SH4s x, float3 direction)
{
    return SH4_dotss(x, SH4s_proj(direction, 1.0f, kPi, kTau / 3.0f));
}

pim_inline float3 VEC_CALL SH4v_irradiance(SH4v x, float3 direction)
{
    return SH4_dotvs(x, SH4s_proj(direction, 1.0f, kPi, kTau / 3.0f));
}

pim_inline SH4v VEC_CALL SH4v_fit(SH4v x, float3 dir, float3 rad, float weight)
{
    SH4v proj = SH4v_proj(dir, rad, 4.0f * kPi, 4.0f * kPi);
    x.v[0] = f3_lerpvs(x.v[0], proj.v[0], weight);
    x.v[1] = f3_lerpvs(x.v[1], proj.v[1], weight);
    x.v[2] = f3_lerpvs(x.v[2], proj.v[2], weight);
    x.v[3] = f3_lerpvs(x.v[3], proj.v[3], weight);
    return x;
}

PIM_C_END
