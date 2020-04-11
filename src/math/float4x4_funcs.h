#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "float4_funcs.h"
#include "float3x3_funcs.h"
#include "math/quat_funcs.h"

static const float4x4 f4x4_0 = 
{
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f }
};

static const float4x4 f4x4_id = 
{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

pim_inline float4x4 VEC_CALL f4x4_transpose(float4x4 m)
{
    float4x4 r;
    r.c0 = f4_v(m.c0.x, m.c1.x, m.c2.x, m.c3.x);
    r.c1 = f4_v(m.c0.y, m.c1.y, m.c2.y, m.c3.y);
    r.c2 = f4_v(m.c0.z, m.c1.z, m.c2.z, m.c3.z);
    r.c3 = f4_v(m.c0.w, m.c1.w, m.c2.w, m.c3.w);
    return r;
}

pim_inline float4x4 VEC_CALL f4x4_mul(float4x4 a, float4x4 b)
{
    float4x4 m;
    float4 x, y, z, w;
    x = f4_mulvs(a.c0, b.c0.x);
    y = f4_mulvs(a.c1, b.c0.y);
    z = f4_mulvs(a.c2, b.c0.z);
    w = f4_mulvs(a.c3, b.c0.w);
    m.c0 = f4_add(f4_add(x, y), f4_add(z, w));
    x = f4_mulvs(a.c0, b.c1.x);
    y = f4_mulvs(a.c1, b.c1.y);
    z = f4_mulvs(a.c2, b.c1.z);
    w = f4_mulvs(a.c3, b.c1.w);
    m.c1 = f4_add(f4_add(x, y), f4_add(z, w));
    x = f4_mulvs(a.c0, b.c2.x);
    y = f4_mulvs(a.c1, b.c2.y);
    z = f4_mulvs(a.c2, b.c2.z);
    w = f4_mulvs(a.c3, b.c2.w);
    m.c2 = f4_add(f4_add(x, y), f4_add(z, w));
    x = f4_mulvs(a.c0, b.c3.x);
    y = f4_mulvs(a.c1, b.c3.y);
    z = f4_mulvs(a.c2, b.c3.z);
    w = f4_mulvs(a.c3, b.c3.w);
    m.c3 = f4_add(f4_add(x, y), f4_add(z, w));
    return m;
}

// assumes dir.w == 0.0f
pim_inline float4 VEC_CALL f4x4_mul_dir(float4x4 m, float4 dir)
{
    float4 a = f4_mulvs(m.c0, dir.x);
    float4 b = f4_mulvs(m.c1, dir.y);
    float4 c = f4_mulvs(m.c2, dir.z);
    return f4_add(a, f4_add(b, c));
}

// assumes pt.w == 1.0f
pim_inline float4 VEC_CALL f4x4_mul_pt(float4x4 m, float4 pt)
{
    float4 a = f4_mulvs(m.c0, pt.x);
    float4 b = f4_mulvs(m.c1, pt.y);
    float4 c = f4_mulvs(m.c2, pt.z);
    return f4_add(f4_add(a, b), f4_add(c, m.c3));
}

// no assumption of col.w
pim_inline float4 VEC_CALL f4x4_mul_col(float4x4 m, float4 col)
{
    float4 a = f4_mulvs(m.c0, col.x);
    float4 b = f4_mulvs(m.c1, col.y);
    float4 c = f4_mulvs(m.c2, col.z);
    float4 d = f4_mulvs(m.c3, col.w);
    return f4_add(f4_add(a, b), f4_add(c, d));
}

pim_inline float4x4 VEC_CALL f4x4_translate(float4x4 m, float3 v)
{
    float4 a = f4_mulvs(m.c0, v.x);
    float4 b = f4_mulvs(m.c1, v.y);
    float4 c = f4_mulvs(m.c2, v.z);
    m.c3 = f4_add(m.c3, f4_add(a, f4_add(b, c)));
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_rotate(float4x4 m, float a, float3 v)
{
    float3x3 rot = f3x3_angle_axis(a, v);
    float4x4 res;
    {
        float4 x, y, z;
        x = f4_mulvs(m.c0, rot.c0.x);
        y = f4_mulvs(m.c1, rot.c0.y);
        z = f4_mulvs(m.c2, rot.c0.z);
        res.c0 = f4_add(x, f4_add(y, z));
        x = f4_mulvs(m.c0, rot.c1.x);
        y = f4_mulvs(m.c1, rot.c1.y);
        z = f4_mulvs(m.c2, rot.c1.z);
        res.c1 = f4_add(x, f4_add(y, z));
        x = f4_mulvs(m.c0, rot.c2.x);
        y = f4_mulvs(m.c1, rot.c2.y);
        z = f4_mulvs(m.c2, rot.c2.z);
        res.c2 = f4_add(x, f4_add(y, z));
        res.c3 = m.c3;
    }
    return res;
}

pim_inline float4x4 VEC_CALL f4x4_scale(float4x4 m, float3 v)
{
    m.c0 = f4_mulvs(m.c0, v.x);
    m.c1 = f4_mulvs(m.c1, v.y);
    m.c2 = f4_mulvs(m.c2, v.z);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_trs(float3 t, quat r, float3 s)
{
    float3x3 rm = quat_f3x3(r);
    rm.c0 = f3_mulvs(rm.c0, s.x);
    rm.c1 = f3_mulvs(rm.c1, s.y);
    rm.c2 = f3_mulvs(rm.c2, s.z);
    float4x4 m;
    m.c0 = f4_v(rm.c0.x, rm.c0.y, rm.c0.z, 0.0f);
    m.c1 = f4_v(rm.c1.x, rm.c1.y, rm.c1.z, 0.0f);
    m.c2 = f4_v(rm.c2.x, rm.c2.y, rm.c2.z, 0.0f);
    m.c3 = f4_v(t.x, t.y, t.z, 1.0f);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_lookat(float3 eye, float3 at, float3 up)
{
    // forward: index finger, straight
    // up: middle finger, bent 90 degrees
    // right: thumb, extended and flat
    // => this is right-handed aka GL
    float3 f = f3_normalize(f3_sub(at, eye)); // front
    float3 s = f3_normalize(f3_cross(f, up)); // side
    float3 u = f3_cross(s, f); // up

    float4x4 m = f4x4_id;
    m.c0.x = s.x;
    m.c1.x = s.y;
    m.c2.x = s.z;
    m.c0.y = u.x;
    m.c1.y = u.y;
    m.c2.y = u.z;
    m.c0.z = -f.x;
    m.c1.z = -f.y;
    m.c2.z = -f.z;
    m.c3.x = -f3_dot(s, eye);
    m.c3.y = -f3_dot(u, eye);
    m.c3.z = f3_dot(f, eye); // not negated (backward == +Z)
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_ortho(
    float left,
    float right,
    float bottom,
    float top,
    float near,
    float far)
{
    float4x4 m = f4x4_id;
    m.c0.x = 2.0f / (right - left);
    m.c1.y = 2.0f / (top - bottom);
    m.c2.z = -2.0f / (far - near);
    m.c3.x = -(right + left) / (right - left);
    m.c3.y = -(top + bottom) / (top - bottom);
    m.c3.z = -(far + near) / (far - near);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_perspective(
    float fovy,
    float aspect,
    float zNear,
    float zFar)
{
    float t = tanf(fovy * 0.5f);

    float4x4 m = f4x4_0;
    m.c0.x = 1.0f / (aspect * t);
    m.c1.y = 1.0f / t;
    m.c2.z = -1.0f * (zFar + zNear) / (zFar - zNear);
    m.c2.w = -1.0f;
    m.c3.z = -2.0f * (zFar * zNear) / (zFar - zNear);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_inf_perspective(
    float fovy,
    float aspect,
    float near)
{
    float range = tanf(fovy * 0.5f) * near;
    float left = -range * aspect;
    float right = range * aspect;
    float bottom = -range;
    float top = range;

    const float e = 1.0f / (1 << 15);
    float4x4 m = f4x4_0;
    m.c0.x = (2.0f * near) / (right - left);
    m.c1.y = (2.0f * near) / (top - bottom);
    m.c2.z = e - 1.0f;
    m.c2.w = -1.0f;
    m.c3.z = (e - 2.0f) * near;
    return m;
}

PIM_C_END
