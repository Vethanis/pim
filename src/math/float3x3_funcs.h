#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "float3_funcs.h"

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

pim_inline float3x3 VEC_CALL f3x3_angle_axis(float angle, float3 axis)
{
    float s = sinf(angle);
    float c = cosf(angle);

    axis = f3_normalize(axis);
    float3 t = f3_mulvs(axis, 1.0f - c);

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

pim_inline float3x3 VEC_CALL f3x3_lookat(float3 eye, float3 at, float3 up)
{
    // forward: index finger, straight
    // up: middle finger, bent 90 degrees
    // right: thumb, extended and flat
    // => this is right-handed aka GL
    float3 f = f3_normalize(f3_sub(at, eye));
    float3 s = f3_normalize(f3_cross(f, up));
    float3 u = f3_cross(s, f);
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

pim_inline float3 VEC_CALL f3x3_mul_col(float3x3 m, float3 col)
{
    float3 a = f3_mulvs(m.c0, col.x);
    float3 b = f3_mulvs(m.c1, col.y);
    float3 c = f3_mulvs(m.c2, col.z);
    return f3_add(a, f3_add(b, c));
}

PIM_C_END
