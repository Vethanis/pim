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

pim_inline float4 VEC_CALL f4x4_mul_extents(float4x4 M, float4 extents)
{
    // f4x4_mul_dir + abs before adding
    float4 a = f4_abs(f4_mulvs(M.c0, extents.x));
    float4 b = f4_abs(f4_mulvs(M.c1, extents.y));
    float4 c = f4_abs(f4_mulvs(M.c2, extents.z));
    return f4_add(f4_add(a, b), c);
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

pim_inline box_t VEC_CALL f4x4_mul_box(float4x4 M, box_t x)
{
    box_t y;
    y.center = f4x4_mul_pt(M, x.center);
    y.extents = f4x4_mul_extents(M, x.extents);
    return y;
}

pim_inline float4x4 VEC_CALL f4x4_translate(float4x4 m, float4 v)
{
    float4 a = f4_mulvs(m.c0, v.x);
    float4 b = f4_mulvs(m.c1, v.y);
    float4 c = f4_mulvs(m.c2, v.z);
    m.c3 = f4_add(m.c3, f4_add(a, f4_add(b, c)));
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_translation(float4 v)
{
    float4x4 m =
    {
        .c0 = f4_v(1.0f, 0.0f, 0.0f, 0.0f),
        .c1 = f4_v(0.0f, 1.0f, 0.0f, 0.0f),
        .c2 = f4_v(0.0f, 0.0f, 1.0f, 0.0f),
        .c3 = f4_v(v.x, v.y, v.z, 1.0f),
    };
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_rotate(float4x4 m, float a, float4 v)
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

pim_inline float4x4 VEC_CALL f4x4_scale(float4x4 m, float4 v)
{
    m.c0 = f4_mulvs(m.c0, v.x);
    m.c1 = f4_mulvs(m.c1, v.y);
    m.c2 = f4_mulvs(m.c2, v.z);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_scaling(float4 v)
{
    float4x4 m =
    {
        .c0 = f4_v(v.x, 0.0f, 0.0f, 0.0f),
        .c1 = f4_v(0.0f, v.y, 0.0f, 0.0f),
        .c2 = f4_v(0.0f, 0.0f, v.z, 0.0f),
        .c3 = f4_v(0.0f, 0.0f, 0.0f, 1.0f),
    };
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_trs(float4 t, quat r, float4 s)
{
    float3x3 rm = quat_f3x3(r);
    rm.c0 = f4_mulvs(rm.c0, s.x);
    rm.c1 = f4_mulvs(rm.c1, s.y);
    rm.c2 = f4_mulvs(rm.c2, s.z);
    float4x4 m;
    m.c0 = f4_v(rm.c0.x, rm.c0.y, rm.c0.z, 0.0f);
    m.c1 = f4_v(rm.c1.x, rm.c1.y, rm.c1.z, 0.0f);
    m.c2 = f4_v(rm.c2.x, rm.c2.y, rm.c2.z, 0.0f);
    m.c3 = f4_v(t.x, t.y, t.z, 1.0f);
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_lookat(float4 eye, float4 at, float4 up)
{
    // forward: index finger, straight
    // up: middle finger, bent 90 degrees
    // right: thumb, extended and flat
    // => this is right-handed aka GL
    float4 f = f4_normalize3(f4_sub(at, eye)); // front
    float4 s = f4_normalize3(f4_cross3(f, up)); // side
    float4 u = f4_cross3(s, f); // up

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
    m.c3.x = -f4_dot3(s, eye);
    m.c3.y = -f4_dot3(u, eye);
    m.c3.z = f4_dot3(f, eye); // not negated (backward == +Z)
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

pim_inline float4x4 VEC_CALL f4x4_inverse(float4x4 m)
{
    float c00 = m.c2.z * m.c3.w - m.c3.z * m.c2.w;
    float c02 = m.c1.z * m.c3.w - m.c3.z * m.c1.w;
    float c03 = m.c1.z * m.c2.w - m.c2.z * m.c1.w;

    float c04 = m.c2.y * m.c3.w - m.c3.y * m.c2.w;
    float c06 = m.c1.y * m.c3.w - m.c3.y * m.c1.w;
    float c07 = m.c1.y * m.c2.w - m.c2.y * m.c1.w;

    float c08 = m.c2.y * m.c3.z - m.c3.y * m.c2.z;
    float c10 = m.c1.y * m.c3.z - m.c3.y * m.c1.z;
    float c11 = m.c1.y * m.c2.z - m.c2.y * m.c1.z;

    float c12 = m.c2.x * m.c3.w - m.c3.x * m.c2.w;
    float c14 = m.c1.x * m.c3.w - m.c3.x * m.c1.w;
    float c15 = m.c1.x * m.c2.w - m.c2.x * m.c1.w;

    float c16 = m.c2.x * m.c3.z - m.c3.x * m.c2.z;
    float c18 = m.c1.x * m.c3.z - m.c3.x * m.c1.z;
    float c19 = m.c1.x * m.c2.z - m.c2.x * m.c1.z;

    float c20 = m.c2.x * m.c3.y - m.c3.x * m.c2.y;
    float c22 = m.c1.x * m.c3.y - m.c3.x * m.c1.y;
    float c23 = m.c1.x * m.c2.y - m.c2.x * m.c1.y;

    float4 f0 = { c00, c00, c02, c03 };
    float4 f1 = { c04, c04, c06, c07 };
    float4 f2 = { c08, c08, c10, c11 };
    float4 f3 = { c12, c12, c14, c15 };
    float4 f4 = { c16, c16, c18, c19 };
    float4 f5 = { c20, c20, c22, c23 };

    float4 v0 = { m.c1.x, m.c0.x, m.c0.x, m.c0.x };
    float4 v1 = { m.c1.y, m.c0.y, m.c0.y, m.c0.y };
    float4 v2 = { m.c1.z, m.c0.z, m.c0.z, m.c0.z };
    float4 v3 = { m.c1.w, m.c0.w, m.c0.w, m.c0.w };

    float4 i0 =
            f4_add(
            f4_sub(
            f4_mul(v1, f0),
            f4_mul(v2, f1)),
            f4_mul(v3, f2));
    float4 i1 =
            f4_add(
            f4_sub(
            f4_mul(v0, f0),
            f4_mul(v2, f3)),
            f4_mul(v3, f4));
    float4 i2 =
            f4_add(
            f4_sub(
            f4_mul(v0, f1),
            f4_mul(v1, f3)),
            f4_mul(v3, f5));
    float4 i3 =
            f4_add(
            f4_sub(
            f4_mul(v0, f2),
            f4_mul(v1, f4)),
            f4_mul(v2, f5));

    const float4 s1 = { +1.0f, -1.0f, +1.0f, -1.0f };
    const float4 s2 = { -1.0f, +1.0f, -1.0f, +1.0f };
    float4x4 inv = 
    {
        f4_mul(i0, s1),
        f4_mul(i1, s2),
        f4_mul(i2, s1),
        f4_mul(i3, s2),
    };

    float4 r0 = { inv.c0.x, inv.c1.x, inv.c2.x, inv.c3.x };
    float det = f4_sum(f4_mul(m.c0, r0));
    ASSERT(det != 0.0f);
    float rcpDet = 1.0f / det;

    inv.c0 = f4_mulvs(inv.c0, rcpDet);
    inv.c1 = f4_mulvs(inv.c1, rcpDet);
    inv.c2 = f4_mulvs(inv.c2, rcpDet);
    inv.c3 = f4_mulvs(inv.c3, rcpDet);

    return inv;
}

PIM_C_END
