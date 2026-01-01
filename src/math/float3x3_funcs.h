#pragma once

#include "math/types.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/float4_funcs.h"

static const float3x3 f3x3_0 =
{
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
};

static const float3x3 f3x3_id =
{
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
};

// [column][row]
#define f3x3_00(m) (m).c0.x
#define f3x3_01(m) (m).c0.y
#define f3x3_02(m) (m).c0.z
#define f3x3_10(m) (m).c1.x
#define f3x3_11(m) (m).c1.y
#define f3x3_12(m) (m).c1.z
#define f3x3_20(m) (m).c2.x
#define f3x3_21(m) (m).c2.y
#define f3x3_22(m) (m).c2.z

pim_inline float3x3 VEC_CALL f4x4_f3x3(float4x4 x)
{
    float3x3 y;
    y.c0 = x.c0;
    y.c1 = x.c1;
    y.c2 = x.c2;
    return y;
}

pim_inline float4x4 VEC_CALL f3x3_f4x4(float3x3 x)
{
    float4x4 y;
    y.c0 = x.c0;
    y.c1 = x.c1;
    y.c2 = x.c2;
    y.c3 = f4_v(0.0f, 0.0f, 0.0f, 1.0f);
    return y;
}

pim_inline float3x3 VEC_CALL f3x3_transpose(float3x3 a)
{
    float3x3 b;

    f3x3_00(b) = f3x3_00(a);
    f3x3_01(b) = f3x3_10(a);
    f3x3_02(b) = f3x3_20(a);
    f3x3_10(b) = f3x3_01(a);
    f3x3_11(b) = f3x3_11(a);
    f3x3_12(b) = f3x3_21(a);
    f3x3_20(b) = f3x3_02(a);
    f3x3_21(b) = f3x3_12(a);
    f3x3_22(b) = f3x3_22(a);

    return b;
}

pim_inline float3x3 VEC_CALL f3x3_inverse(float3x3 m)
{
    float m_00_11_10_01 = f3x3_00(m) * f3x3_11(m) - f3x3_10(m) * f3x3_01(m);
    float m_00_12_10_02 = f3x3_00(m) * f3x3_12(m) - f3x3_10(m) * f3x3_02(m);
    float m_00_21_20_01 = f3x3_00(m) * f3x3_21(m) - f3x3_20(m) * f3x3_01(m);
    float m_00_22_20_02 = f3x3_00(m) * f3x3_22(m) - f3x3_20(m) * f3x3_02(m);
    float m_01_12_11_02 = f3x3_01(m) * f3x3_12(m) - f3x3_11(m) * f3x3_02(m);
    float m_01_22_21_02 = f3x3_01(m) * f3x3_22(m) - f3x3_21(m) * f3x3_02(m);
    float m_10_21_20_11 = f3x3_10(m) * f3x3_21(m) - f3x3_20(m) * f3x3_11(m);
    float m_10_22_20_12 = f3x3_10(m) * f3x3_22(m) - f3x3_20(m) * f3x3_12(m);
    float m_11_22_21_12 = f3x3_11(m) * f3x3_22(m) - f3x3_21(m) * f3x3_12(m);

    float d0 = f3x3_00(m) * m_11_22_21_12;
    float d1 = -f3x3_10(m) * m_01_22_21_02;
    float d2 = f3x3_20(m) * m_01_12_11_02;
    float det = d0 + d1 + d2;
    ASSERT(f1_abs(det) >= kEpsilon);
    float rcpDet = 1.0f / det;

    float3x3 inv = {0};

    f3x3_00(inv) = rcpDet * m_11_22_21_12;
    f3x3_10(inv) = -rcpDet * m_10_22_20_12;
    f3x3_20(inv) = rcpDet * m_10_21_20_11;

    f3x3_01(inv) = -rcpDet * m_01_22_21_02;
    f3x3_11(inv) = rcpDet * m_00_22_20_02;
    f3x3_21(inv) = -rcpDet * m_00_21_20_01;

    f3x3_02(inv) = rcpDet * m_01_12_11_02;
    f3x3_12(inv) = -rcpDet * m_00_12_10_02;
    f3x3_22(inv) = rcpDet * m_00_11_10_01;

    return inv;
}

pim_inline float3x3 VEC_CALL f3x3_IM(float4x4 M)
{
    return f3x3_inverse(f3x3_transpose(f4x4_f3x3(M)));
}

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

pim_inline float3x3 VEC_CALL f3x3_lookat(float4 forward, float4 up)
{
    // right-handed
    float4 right = f4_normalize3(f4_cross3(forward, up));
    up = f4_normalize3(f4_cross3(right, forward));
    float3x3 m =
    {
        { right.x, up.x, -forward.x },
        { right.y, up.y, -forward.y },
        { right.z, up.z, -forward.z },
    };
    return m;
}

pim_inline float4 VEC_CALL f3x3_mul_col(float3x3 m, float4 col)
{
    return f4_add(
            f4_mulvs(m.c0, col.x),
        f4_add(
            f4_mulvs(m.c1, col.y),
            f4_mulvs(m.c2, col.z)));
}

// columns of b are transformed by matrix a
pim_inline float3x3 VEC_CALL f3x3_mul(float3x3 a, float3x3 b)
{
    float3x3 m;
    m.c0 = f3x3_mul_col(a, b.c0);
    m.c1 = f3x3_mul_col(a, b.c1);
    m.c2 = f3x3_mul_col(a, b.c2);
    return m;
}

PIM_C_END
