#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "float4_funcs.h"
#include "float3x3_funcs.h"
#include "math/quat_funcs.h"

// [column][row]
#define f4x4_00(m) (m).c0.x
#define f4x4_01(m) (m).c0.y
#define f4x4_02(m) (m).c0.z
#define f4x4_03(m) (m).c0.w
#define f4x4_10(m) (m).c1.x
#define f4x4_11(m) (m).c1.y
#define f4x4_12(m) (m).c1.z
#define f4x4_13(m) (m).c1.w
#define f4x4_20(m) (m).c2.x
#define f4x4_21(m) (m).c2.y
#define f4x4_22(m) (m).c2.z
#define f4x4_23(m) (m).c2.w
#define f4x4_30(m) (m).c3.x
#define f4x4_31(m) (m).c3.y
#define f4x4_32(m) (m).c3.z
#define f4x4_33(m) (m).c3.w

static const float4x4 f4x4_0 = 
{
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

static const float4x4 f4x4_id = 
{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};

pim_inline float4x4 VEC_CALL f4x4_transpose(const float4x4 m)
{
    float4x4 r;
    r.c0 = f4_v(m.c0.x, m.c1.x, m.c2.x, m.c3.x);
    r.c1 = f4_v(m.c0.y, m.c1.y, m.c2.y, m.c3.y);
    r.c2 = f4_v(m.c0.z, m.c1.z, m.c2.z, m.c3.z);
    r.c3 = f4_v(m.c0.w, m.c1.w, m.c2.w, m.c3.w);
    return r;
}

// no assumption of col.w
pim_inline float4 VEC_CALL f4x4_mul_col(const float4x4 m, const float4 col)
{
    float4 a = f4_mulvs(m.c0, col.x);
    float4 b = f4_mulvs(m.c1, col.y);
    float4 c = f4_mulvs(m.c2, col.z);
    float4 d = f4_mulvs(m.c3, col.w);
    return f4_add(f4_add(a, b), f4_add(c, d));
}

// columns of b are transformed by matrix a
pim_inline float4x4 VEC_CALL f4x4_mul(const float4x4 a, const float4x4 b)
{
    float4x4 m;
    m.c0 = f4x4_mul_col(a, b.c0);
    m.c1 = f4x4_mul_col(a, b.c1);
    m.c2 = f4x4_mul_col(a, b.c2);
    m.c3 = f4x4_mul_col(a, b.c3);
    return m;
}

// assumes dir.w == 0.0f
pim_inline float4 VEC_CALL f4x4_mul_dir(const float4x4 m, const float4 dir)
{
    float4 a = f4_mulvs(m.c0, dir.x);
    float4 b = f4_mulvs(m.c1, dir.y);
    float4 c = f4_mulvs(m.c2, dir.z);
    return f4_add(a, f4_add(b, c));
}

// assumes pt.w == 1.0f
pim_inline float4 VEC_CALL f4x4_mul_pt(const float4x4 m, const float4 pt)
{
    float4 a = f4_mulvs(m.c0, pt.x);
    float4 b = f4_mulvs(m.c1, pt.y);
    float4 c = f4_mulvs(m.c2, pt.z);
    return f4_add(f4_add(a, b), f4_add(c, m.c3));
}

pim_inline float4 VEC_CALL f4x4_mul_extents(const float4x4 m, const float4 extents)
{
    // f4x4_mul_dir + abs before adding
    float4 a = f4_abs(f4_mulvs(m.c0, extents.x));
    float4 b = f4_abs(f4_mulvs(m.c1, extents.y));
    float4 c = f4_abs(f4_mulvs(m.c2, extents.z));
    return f4_add(a, f4_add(b, c));
}

pim_inline float4x4 VEC_CALL f4x4_mul_scalar(const float4x4 m, const float s)
{
    float4x4 n =
    {
        f4_mulvs(m.c0, s),
        f4_mulvs(m.c1, s),
        f4_mulvs(m.c2, s),
        f4_mulvs(m.c3, s),
    };
    return n;
}

pim_inline float4x4 VEC_CALL f4x4_translate(const float4x4 m, const float4 v)
{
    float4x4 n =
    {
        m.c0,
        m.c1,
        m.c2,
        f4x4_mul_pt(m, v),
    };
    return n;
}

pim_inline float4x4 VEC_CALL f4x4_translation(const float4 v)
{
    float4x4 m;
    m.c0 = f4_v(1.0f, 0.0f, 0.0f, 0.0f);
    m.c1 = f4_v(0.0f, 1.0f, 0.0f, 0.0f);
    m.c2 = f4_v(0.0f, 0.0f, 1.0f, 0.0f);
    m.c3 = f4_v(v.x, v.y, v.z, 1.0f);
    return m;
}

pim_inline float4 VEC_CALL f4x4_derive_translation(const float4x4 m)
{
    return m.c3;
}

pim_inline float4x4 VEC_CALL f4x4_rotate(const float4x4 m, const quat q)
{
    float4x4 res;
    {
        float3x3 rot = quat_f3x3(q);
        res.c0 = f4x4_mul_dir(m, rot.c0);
        res.c1 = f4x4_mul_dir(m, rot.c1);
        res.c2 = f4x4_mul_dir(m, rot.c2);
        res.c3 = m.c3;
    }
    return res;
}

pim_inline quat VEC_CALL f4x4_derive_rotation(const float4x4 m)
{
    // maybe wrong
    return f3x3_quat(f4x4_f3x3(m));
}

pim_inline float4x4 VEC_CALL f4x4_scale(const float4x4 m, const float4 v)
{
    float4x4 n;
    n.c0 = f4_mulvs(m.c0, v.x);
    n.c1 = f4_mulvs(m.c1, v.y);
    n.c2 = f4_mulvs(m.c2, v.z);
    n.c3 = m.c3;
    return n;
}

pim_inline float4x4 VEC_CALL f4x4_scaling(const float4 v)
{
    float4x4 m;
    m.c0 = f4_v(v.x, 0.0f, 0.0f, 0.0f);
    m.c1 = f4_v(0.0f, v.y, 0.0f, 0.0f);
    m.c2 = f4_v(0.0f, 0.0f, v.z, 0.0f);
    m.c3 = f4_v(0.0f, 0.0f, 0.0f, 1.0f);
    return m;
}

pim_inline float4 VEC_CALL f4x4_derive_scale(const float4x4 m)
{
    // probably wrong.
    // ignores negative scale.
    return f4_v(f4_length3(m.c0), f4_length3(m.c1), f4_length3(m.c2), 0.0f);
}

pim_inline float4x4 VEC_CALL f4x4_trs(const float4 t, const quat r, const float4 s)
{
    return f4x4_scale(f4x4_rotate(f4x4_translation(t), r), s);
}

pim_inline float4x4 VEC_CALL f4x4_lookat(const float4 eye, const float4 at, const float4 up)
{
    // right-handed
    float4 f = f4_normalize3(f4_sub(at, eye));
    float4 s = f4_normalize3(f4_cross3(f, up));
    float4 u = f4_normalize3(f4_cross3(s, f));

    float tx = -f4_dot3(s, eye);
    float ty = -f4_dot3(u, eye);
    float tz = f4_dot3(f, eye);
    float4x4 m =
    {
        { s.x, u.x, -f.x, 0.0f },
        { s.y, u.y, -f.y, 0.0f },
        { s.z, u.z, -f.z, 0.0f },
        { tx, ty, tz, 1.0f },
    };
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

pim_inline float4x4 VEC_CALL f4x4_glperspective(
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

pim_inline float4x4 VEC_CALL f4x4_vkperspective(
    float fovy,
    float aspect,
    float zNear,
    float zFar)
{
    float4x4 m = f4x4_glperspective(fovy, aspect, zNear, zFar);
    f4x4_11(m) *= -1.0f;
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

    float4x4 m = f4x4_0;
    m.c0.x = (2.0f * near) / (right - left);
    m.c1.y = (2.0f * near) / (top - bottom);
    m.c2.z = kEpsilon - 1.0f;
    m.c2.w = -1.0f;
    m.c3.z = (kEpsilon - 2.0f) * near;
    return m;
}

pim_inline float4x4 VEC_CALL f4x4_inverse(const float4x4 m)
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

    const float4 f0 = { c00, c00, c02, c03 };
    const float4 f1 = { c04, c04, c06, c07 };
    const float4 f2 = { c08, c08, c10, c11 };
    const float4 f3 = { c12, c12, c14, c15 };
    const float4 f4 = { c16, c16, c18, c19 };
    const float4 f5 = { c20, c20, c22, c23 };

    const float4 v0 = { m.c1.x, m.c0.x, m.c0.x, m.c0.x };
    const float4 v1 = { m.c1.y, m.c0.y, m.c0.y, m.c0.y };
    const float4 v2 = { m.c1.z, m.c0.z, m.c0.z, m.c0.z };
    const float4 v3 = { m.c1.w, m.c0.w, m.c0.w, m.c0.w };

    const float4 i0 =
            f4_add(
            f4_sub(
            f4_mul(v1, f0),
            f4_mul(v2, f1)),
            f4_mul(v3, f2));
    const float4 i1 =
            f4_add(
            f4_sub(
            f4_mul(v0, f0),
            f4_mul(v2, f3)),
            f4_mul(v3, f4));
    const float4 i2 =
            f4_add(
            f4_sub(
            f4_mul(v0, f1),
            f4_mul(v1, f3)),
            f4_mul(v3, f5));
    const float4 i3 =
            f4_add(
            f4_sub(
            f4_mul(v0, f2),
            f4_mul(v1, f4)),
            f4_mul(v2, f5));

    const float4 s1 = { +1.0f, -1.0f, +1.0f, -1.0f };
    const float4 s2 = { -1.0f, +1.0f, -1.0f, +1.0f };
    const float4x4 inv = 
    {
        f4_mul(i0, s1),
        f4_mul(i1, s2),
        f4_mul(i2, s1),
        f4_mul(i3, s2),
    };

    const float4 r0 = { inv.c0.x, inv.c1.x, inv.c2.x, inv.c3.x };
    float det = f4_sum(f4_mul(m.c0, r0));
    ASSERT(det != 0.0f);
    float rcpDet = 1.0f / det;

    return f4x4_mul_scalar(inv, rcpDet);
}

// calculates an eigenvector
// Richard von Mises and H. Pollaczek-Geiringer
//    Praktische Verfahren der Gleichungsauflösung
//    ZAMM - Zeitschrift für Angewandte Mathematik und Mechanik 9, 152-164 (1929).
pim_inline float4 VEC_CALL f4x4_power_iteration(
    const float4x4 m,
    const float4 guess, // eg. random vector of non-zero values
    float maxErrSq)
{
    ASSERT(guess.x > kEpsilon && guess.y > kEpsilon && guess.z > kEpsilon && guess.w > kEpsilon);
    float4 v1 = guess;
    float errSq;
    do
    {
        const float4 v0 = v1;
        v1 = f4_normalize4(f4x4_mul_col(m, v0));
        errSq = f4_distancesq4(v0, v1);
    } while (errSq > maxErrSq);
    return v1;
}

PIM_C_END
