#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"

pim_inline float4 VEC_CALL ray_tri_isect(
    float3 ro, float3 rd,
    float3 A, float3 B, float3 C)
{
    float4 result = { 0.0f, 0.0f, 0.0f, 0.0f };
    float3 BA = f3_sub(B, A);
    float3 CA = f3_sub(C, A);
    float3 P = f3_cross(rd, CA);
    float det = f3_dot(BA, P);
    if (det >= f16_eps)
    {
        float rcpDet = 1.0f / det;
        float3 T = f3_sub(ro, A);
        float3 Q = f3_cross(T, BA);

        float t = f3_dot(CA, Q) * rcpDet;
        float u = f3_dot(T, P) * rcpDet;
        float v = f3_dot(rd, Q) * rcpDet;

        float tU = (u >= 0.0f) ? 1.0f : 0.0f;
        float tV = (v >= 0.0f) ? 1.0f : 0.0f;
        float tUV = ((u + v) <= 1.0f) ? 1.0f : 0.0f;
        float tT = (t > 0.0f) ? 1.0f : 0.0f;

        result.x = u;
        result.y = v;
        result.z = t;
        result.w = (tU + tV + tUV + tT) * 0.25f;
    }
    return result;
}

pim_inline float VEC_CALL sdTriangle2D(
    float2 a, float2 b, float2 c, float2 p)
{
    float2 ba = f2_sub(b, a); float2 pa = f2_sub(p, a);
    float2 cb = f2_sub(c, b); float2 pb = f2_sub(p, b);
    float2 ac = f2_sub(a, c); float2 pc = f2_sub(p, c);

    float h = f1_saturate(f2_dot(ba, pa) / f2_lengthsq(ba));
    h = f2_lengthsq(f2_sub(f2_mulvs(ba, h), pa));

    float i = f1_saturate(f2_dot(cb, pb) / f2_lengthsq(cb));
    i = f2_lengthsq(f2_sub(f2_mulvs(cb, i), pb));

    float j = f1_saturate(f2_dot(ac, pc) / f2_lengthsq(ac));
    j = f2_lengthsq(f2_sub(f2_mulvs(ac, j), pc));

    return sqrtf(f1_min(h, f1_min(i, j)));
}

pim_inline float VEC_CALL sdPlane2D(float2 n, float d, float2 pt)
{
    return f2_dot(n, pt) - d;
}

pim_inline float VEC_CALL sdPlaneBox2D(
    float2 n, float d, float2 center, float2 extents)
{
    return sdPlane2D(n, d, center) - f1_abs(sdPlane2D(n, 0.0f, extents));
}

// pineda edge function (flipped for right handedness)
// returns > 0 when on right side of A -> B
// returns 0 when exactly on A -> B
// returns < 0 when on left side of A -> B
// returns the area of the triangle formed by A -> B -> P
// (also is its determinant aka cross product in two dimensions)
pim_inline float VEC_CALL tri_area2D(float2 A, float2 B, float2 P)
{
    float2 AP = f2_sub(P, A);
    float2 AB = f2_sub(B, A);
    // the determinant of
    // [AP.x, AP.y]
    // [AB.x, AB.y]
    float e = AP.y * AB.x - AP.x * AB.y;
    return 0.5f * e;
}

// calculates barycentric coordinates of p on triangle abc
// rcpArea = 1.0f / tri_area2D(a, b, c);
pim_inline float3 VEC_CALL bary2D(float2 a, float2 b, float2 c, float rcpArea, float2 p)
{
    const float w = tri_area2D(b, c, p) * rcpArea;
    const float u = tri_area2D(c, a, p) * rcpArea;
    const float v = tri_area2D(a, b, p) * rcpArea;
    return f3_v(w, u, v);
}

PIM_C_END
