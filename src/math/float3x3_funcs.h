#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/float4_funcs.h"

static const float3x3 f3x3_0 =
{
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f }
};

static const float3x3 f3x3_id =
{
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f }
};

pim_inline float3x3 VEC_CALL f3x3_angle_axis(float angle, float4 axis)
{
    float s = sinf(angle);
    float c = cosf(angle);

    axis = f4_normalize3(axis);
    float4 t = f4_mulvs(axis, 1.0f - c);

    float3x3 m;
    m.c0.x = c + t.x * axis.x;
    m.c0.y = t.x * axis.y + s * axis.z;
    m.c0.z = t.x * axis.z - s * axis.y;

    m.c1.x = t.y * axis.x - s * axis.z;
    m.c1.y = c + t.y * axis.y;
    m.c1.z = t.y * axis.z + s * axis.x;

    m.c2.x = t.z * axis.x + s * axis.y;
    m.c2.y = t.z * axis.y - s * axis.x;
    m.c2.z = c + t.z * axis.z;

    return m;
}

pim_inline float3x3 VEC_CALL f3x3_lookat(float4 eye, float4 at, float4 up)
{
    // forward: index finger, straight
    // up: middle finger, bent 90 degrees
    // right: thumb, extended and flat
    // => this is right-handed aka GL
    float4 f = f4_normalize3(f4_sub(at, eye));
    float4 s = f4_normalize3(f4_cross3(f, up));
    float4 u = f4_cross3(s, f);
    float3x3 m;
    m.c0.x = s.x;
    m.c1.x = s.y;
    m.c2.x = s.z;
    m.c0.y = u.x;
    m.c1.y = u.y;
    m.c2.y = u.z;
    m.c0.z = -f.x;
    m.c1.z = -f.y;
    m.c2.z = -f.z;
    return m;
}

pim_inline float4 VEC_CALL f3x3_mul_col(float3x3 m, float4 col)
{
    float4 a = f4_mulvs(m.c0, col.x);
    float4 b = f4_mulvs(m.c1, col.y);
    float4 c = f4_mulvs(m.c2, col.z);
    return f4_add(a, f4_add(b, c));
}

PIM_C_END
