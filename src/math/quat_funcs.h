#pragma once
#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/float3x3_funcs.h"
#include "math/float4_funcs.h"

#define quat_0 quat_v(0.0f, 0.0f, 0.0f, 0.0f)
#define quat_id quat_v(0.0f, 0.0f, 0.0f, 1.0f)

pim_inline quat VEC_CALL f4_quat(float4 f)
{
    quat q = { f };
    return q;
}

pim_inline quat VEC_CALL quat_v(float x, float y, float z, float w)
{
    quat q = { { x, y, z, w } };
    return q;
}

pim_inline float VEC_CALL quat_dot(quat a, quat b)
{
    return f4_dot4(a.v, b.v);
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
    return f4_quat(f4_lerpvs(a.v, b.v, t));
}

pim_inline quat VEC_CALL quat_slerp(quat a, quat b, float t)
{
    float cosTheta = quat_dot(a, b);
    if (cosTheta < 0.0f)
    {
        cosTheta = -cosTheta;
        b.v = f4_neg(b.v);
    }
    if ((cosTheta * cosTheta) >= 0.99f)
    {
        return quat_lerp(a, b, t);
    }
    else
    {
        float theta = acosf(cosTheta);
        float t0 = sinf((1.0f - t) * theta);
        float t1 = sinf(t * theta);
        float scale = 1.0f / sinf(theta);
        return f4_quat(
            f4_mulvs(
                f4_add(
                    f4_mulvs(a.v, t0),
                    f4_mulvs(b.v, t1)),
                scale));
    }
}

pim_inline quat VEC_CALL quat_mul(quat a, quat b)
{
    quat c;
    c.v.x = a.v.w * b.v.x + a.v.x * b.v.w + a.v.y * b.v.z - a.v.z * b.v.y;
    c.v.y = a.v.w * b.v.y + a.v.y * b.v.w + a.v.z * b.v.x - a.v.x * b.v.z;
    c.v.z = a.v.w * b.v.z + a.v.z * b.v.w + a.v.x * b.v.y - a.v.y * b.v.x;
    c.v.w = a.v.w * b.v.w - a.v.x * b.v.x - a.v.y * b.v.y - a.v.z * b.v.z;
    return c;
}

pim_inline float4 VEC_CALL quat_mul_dir(quat q, float4 dir)
{
    float4 uv = f4_cross3(q.v, dir);
    float4 uuv = f4_cross3(q.v, uv);

    float4 y = f4_mulvs(uv, q.v.w);
    y = f4_add(y, uuv);
    y = f4_mulvs(y, 2.0f);
    y = f4_add(y, dir);

    return y;
}

pim_inline float4 VEC_CALL quat_fwd(quat q)
{
    float4 dir = { 0.0f, 0.0f, -1.0f, 0.0f };
    return quat_mul_dir(q, dir);
}

pim_inline float4 VEC_CALL quat_up(quat q)
{
    float4 dir = { 0.0f, 1.0f, 0.0f, 0.0f };
    return quat_mul_dir(q, dir);
}

pim_inline float4 VEC_CALL quat_right(quat q)
{
    float4 dir = { 1.0f, 0.0f, 0.0f, 0.0f };
    return quat_mul_dir(q, dir);
}

pim_inline float4 VEC_CALL quat_mul_invdir(float4 dir, quat q)
{
    return quat_mul_dir(quat_inverse(q), dir);
}

pim_inline quat VEC_CALL quat_angleaxis(float angle, float4 axis)
{
    angle *= 0.5f;
    float s = sinf(angle);
    float c = cosf(angle);
    axis = f4_mulvs(axis, s);
    return quat_v(axis.x, axis.y, axis.z, c);
}

pim_inline float3x3 VEC_CALL quat_f3x3(quat q)
{
    float4 v = q.v;
    float xx = v.x * v.x;
    float xy = v.x * v.y;
    float xz = v.x * v.z;
    float yy = v.y * v.y;
    float yz = v.y * v.z;
    float zz = v.z * v.z;
    float wx = v.w * v.x;
    float wy = v.w * v.y;
    float wz = v.w * v.z;

    float3x3 m;
    m.c0.x = 1.0f - 2.0f * (yy + zz);
    m.c0.y = 2.0f * (xy - wz);
    m.c0.z = 2.0f * (xz + wy);

    m.c1.x = 2.0f * (xy + wz);
    m.c1.y = 1.0f - 2.0f * (xx + zz);
    m.c1.z = 2.0f * (yz - wx);

    m.c2.x = 2.0f * (xz - wy);
    m.c2.y = 2.0f * (yz + wx);
    m.c2.z = 1.0f - 2.0f * (xx + yy);

    return m;
}

pim_inline quat VEC_CALL f3x3_quat(float3x3 m)
{
    float x = f3x3_00(m) - f3x3_11(m) - f3x3_22(m);
    float y = f3x3_11(m) - f3x3_00(m) - f3x3_22(m);
    float z = f3x3_22(m) - f3x3_00(m) - f3x3_11(m);
    float w = f3x3_00(m) + f3x3_11(m) + f3x3_22(m);
    float b = f1_max(f1_max(x, y), f1_max(z, w));

    i32 i = 0;
    i = (b == y) ? 1 : i;
    i = (b == z) ? 2 : i;
    i = (b == w) ? 3 : i;

    b = sqrtf(b + 1.0f) * 0.5f;
    float mult = 0.25f / b;

    float c0 = (f3x3_10(m) + f3x3_01(m)) * mult;
    float c1 = (f3x3_10(m) - f3x3_01(m)) * mult;
    float c2 = (f3x3_21(m) + f3x3_12(m)) * mult;
    float c3 = (f3x3_21(m) - f3x3_12(m)) * mult;
    float c4 = (f3x3_02(m) + f3x3_20(m)) * mult;
    float c5 = (f3x3_02(m) - f3x3_20(m)) * mult;

    float4 r = quat_id.v;
    switch (i)
    {
    default:
        ASSERT(false);
        break;
    case 0:
    {
        // b == x
        r.x = b;
        r.y = c0;
        r.z = c4;
        r.w = c3;
    }
    break;
    case 1:
    {
        // b == y
        r.x = c0;
        r.y = b;
        r.z = c2;
        r.w = c5;
    }
    break;
    case 2:
    {
        // b == z
        r.x = c4;
        r.y = c2;
        r.z = b;
        r.w = c1;
    }
    break;
    case 3:
    {
        // b == w
        r.x = c3;
        r.y = c5;
        r.z = c1;
        r.w = b;
    }
    break;
    }

    return f4_quat(r);
}

pim_inline quat VEC_CALL quat_lookat(float4 forward, float4 up)
{
    return f3x3_quat(f3x3_lookat(forward, up));
}

PIM_C_END
