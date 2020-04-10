#pragma once
#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/float3x3_funcs.h"
#include "math/float4_funcs.h"

static const quat quat_0 = { 0.0f, 0.0f, 0.0f, 0.0f };
static const quat quat_id = { 0.0f, 0.0f, 0.0f, 1.0f };

pim_inline quat VEC_CALL f4_quat(float4 f)
{
    quat q = { f };
    return q;
}

pim_inline quat VEC_CALL quat_v(float x, float y, float z, float w)
{
    quat q = { x, y, z, w };
    return q;
}

pim_inline float VEC_CALL quat_dot(quat a, quat b)
{
    return f4_dot(a.v, b.v);
}

pim_inline quat VEC_CALL quat_conjugate(quat q)
{
    // w doesnt flip sign
    return quat_v(-q.v.x, -q.v.y, -q.v.z, q.v.w);
}

pim_inline quat VEC_CALL quat_inverse(quat q)
{
    quat conj = quat_conjugate(q);
    float t = 1.0f / quat_dot(q, q);
    quat r = { f4_mulvs(conj.v, t) };
    return r;
}

pim_inline quat VEC_CALL quat_lerp(quat a, quat b, float t)
{
    return f4_quat(f4_lerp(a.v, b.v, t));
}

pim_inline quat VEC_CALL quat_slerp(quat a, quat b, float t)
{
    float cosTheta = quat_dot(a, b);
    if (cosTheta < 0.0f)
    {
        cosTheta = -cosTheta;
        b.v = f4_neg(b.v);
    }
    if ((1.0f - fabsf(cosTheta)) < 0.001f)
    {
        return quat_lerp(a, b, t);
    }
    else
    {
        float theta = acosf(cosTheta);
        float t0 = sinf((1.0f - t) * theta);
        float t1 = sinf(t * theta);
        float scale = 1.0f / sinf(theta);

        float4 a4 = f4_mulvs(a.v, t0);
        float4 b4 = f4_mulvs(b.v, t1);
        float4 c4 = f4_mulvs(f4_add(a4, b4), scale);
        return f4_quat(c4);
    }
}

pim_inline quat VEC_CALL quat_mul(quat a, quat b)
{
    //c.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    //c.y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
    //c.z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
    //c.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;

    float4 c, ca, cb;
    c = f4_mul(f4_s(a.v.w), b.v);

    ca = f4_v(a.v.x, a.v.y, a.v.z, -a.v.x);
    cb = f4_v(b.v.w, b.v.w, b.v.w, b.v.x);
    c = f4_add(c, f4_mul(ca, cb));

    ca = f4_v(a.v.y, a.v.z, a.v.x, -a.v.y);
    cb = f4_v(b.v.z, b.v.x, b.v.y, b.v.y);
    c = f4_add(c, f4_mul(ca, cb));

    ca = f4_v(-a.v.z, -a.v.x, -a.v.y, -a.v.z);
    cb = f4_v(b.v.y, b.v.z, b.v.x, b.v.z);
    c = f4_add(c, f4_mul(ca, cb));

    return f4_quat(c);
}

pim_inline float3 VEC_CALL quat_mul_dir(quat q, float3 dir)
{
    float3 q3 = f3_v(q.v.x, q.v.y, q.v.z);
    float3 uv = f3_cross(q3, dir);
    float3 uuv = f3_cross(q3, uv);

    float3 y = f3_mulvs(uv, q.v.w);
    y = f3_add(y, uuv);
    y = f3_mulvs(y, 2.0f);
    y = f3_add(y, dir);

    return y;
}

pim_inline float3 VEC_CALL quat_mul_invdir(float3 dir, quat q)
{
    return quat_mul_dir(quat_inverse(q), dir);
}

pim_inline quat VEC_CALL quat_angleaxis(float angle, float3 axis)
{
    angle *= 0.5f;
    float s = sinf(angle);
    float c = cosf(angle);
    axis = f3_mulvs(axis, s);
    return quat_v(axis.x, axis.y, axis.z, c);
}

pim_inline float3x3 VEC_CALL quat_f3x3(quat q)
{
    float4 v = q.v;
    float xx = v.x * v.x;
    float yy = v.y * v.y;
    float zz = v.z * v.z;
    float xz = v.x * v.z;
    float xy = v.x * v.y;
    float yz = v.y * v.z;
    float wx = v.w * v.x;
    float wy = v.w * v.y;
    float wz = v.w * v.z;

    float3x3 m;
    m.c0.x = 1.0f - 2.0f * (yy + zz);
    m.c0.y = 2.0f * (xy + wz);
    m.c0.z = 2.0f * (xz - wy);

    m.c1.x = 2.0f * (xy - wz);
    m.c1.y = 1.0f - 2.0f * (xx + zz);
    m.c1.z = 2.0f * (yz + wx);

    m.c2.x = 2.0f * (xz + wy);
    m.c2.y = 2.0f * (yz - wx);
    m.c2.z = 1.0f - 2.0f * (xx + yy);

    return m;
}

pim_inline quat VEC_CALL f3x3_quat(float3x3 m)
{
    float x = m.c0.x - m.c1.y - m.c2.z;
    float y = m.c1.y - m.c0.x - m.c2.z;
    float z = m.c2.z - m.c0.x - m.c1.y;
    float w = m.c0.x + m.c1.y + m.c2.z;

    i32 i = 0;
    float b = w;
    b = f32_max(b, x);
    b = f32_max(b, z);
    b = f32_max(b, y);
    i = (b == x) ? 1 : i;
    i = (b == y) ? 2 : i;
    i = (b == z) ? 3 : i;

    b = sqrtf(b + 1.0f) * 0.5f;
    float mult = 0.25f / b;

    float4 r = quat_id.v;

    switch (i)
    {
    case 0:
    {
        // b == w
        r.w = b;
        r.x = (m.c1.z - m.c2.y) * mult;
        r.y = (m.c2.x - m.c0.z) * mult;
        r.z = (m.c0.y - m.c1.x) * mult;
    }
    break;
    case 1:
    {
        // b == x
        r.w = (m.c1.z - m.c2.y) * mult;
        r.x = b;
        r.y = (m.c0.y + m.c1.x) * mult;
        r.z = (m.c2.x + m.c0.z) * mult;
    }
    break;
    case 2:
    {
        // b == y
        r.w = (m.c2.x - m.c0.z) * mult;
        r.x = (m.c0.y + m.c1.x) * mult;
        r.y = b;
        r.z = (m.c1.z + m.c2.y) * mult;
    }
    break;
    case 3:
    {
        // b == z
        r.w = (m.c0.y - m.c1.x) * mult;
        r.x = (m.c2.x + m.c0.z) * mult;
        r.y = (m.c1.z + m.c2.y) * mult;
        r.z = b;
    }
    break;
    default:
        ASSERT(false);
        break;
    }

    return f4_quat(r);
}

pim_inline quat VEC_CALL quat_lookat(float3 forward, float3 up)
{
    float3x3 m;
    m.c2 = f3_neg(forward);
    m.c0 = f3_normalize(f3_cross(up, m.c2));
    m.c1 = f3_cross(m.c2, m.c0);
    return f3x3_quat(m);
}

PIM_C_END
