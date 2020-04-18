#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"

pim_inline float VEC_CALL sdCircle(float2 center, float radius, float2 pt)
{
    return f2_distance(center, pt) - radius;
}

pim_inline float VEC_CALL sdSphere(float3 center, float radius, float3 pt)
{
    return f3_distance(center, pt) - radius;
}

pim_inline float VEC_CALL sdPlane2D(float2 n, float d, float2 pt)
{
    return f2_dot(n, pt) - d;
}

pim_inline float VEC_CALL sdPlane3D(float4 plane, float3 pt)
{
    return f4_dot(plane, f4_v(pt.x, pt.y, pt.z, 0.0f)) - plane.w;
}

pim_inline float VEC_CALL sdLine2D(float2 a, float2 b, float2 pt)
{
    float2 pa = f2_sub(pt, a);
    float2 ba = f2_sub(b, a);
    float h = f1_saturate(f2_dot(pa, ba) / f2_lengthsq(ba));
    return f2_length(f2_sub(pa, f2_mulvs(ba, h)));
}

pim_inline float VEC_CALL sdLine3D(float3 a, float3 b, float r, float3 pt)
{
    float3 pa = f3_sub(pt, a);
    float3 ba = f3_sub(b, a);
    float h = f1_saturate(f3_dot(pa, ba) / f3_lengthsq(ba));
    return f3_length(f3_sub(pa, f3_mulvs(ba, h))) - r;
}

pim_inline float VEC_CALL sdBox2D(float2 c, float2 e, float2 pt)
{
    float2 d = f2_sub(f2_abs(f2_sub(pt, c)), e);
    return f2_length(f2_max(d, f2_0)) + f1_min(f2_hmax(d), 0.0f);
}

pim_inline float VEC_CALL sdBox3D(float3 c, float3 e, float3 pt)
{
    float3 d = f3_sub(f3_abs(f3_sub(pt, c)), e);
    return f3_length(f3_max(d, f3_0)) + f1_min(f3_hmax(d), 0.0f);
}

pim_inline float VEC_CALL sdPlaneCircle(float2 n, float d, float2 c, float r)
{
    return sdPlane2D(n, d, c) - r;
}

pim_inline float VEC_CALL sdPlaneSphere(float4 plane, float4 sphere)
{
    return sdPlane3D(plane, f4_f3(sphere)) - sphere.w;
}

pim_inline float VEC_CALL sdPlaneBox2D(float2 n, float d, float2 c, float2 e)
{
    return sdPlane2D(n, d, c) - f1_abs(sdPlane2D(n, 0.0f, e));
}

pim_inline float VEC_CALL sdPlaneBox3D(float4 plane, float3 c, float3 e)
{
    float a = sdPlane3D(plane, c);
    plane.w = 0.0f;
    float b = f1_abs(sdPlane3D(plane, e));
    return a - b;
}

// pineda edge function (flipped for right handedness)
// returns > 0 when on right side of A -> B
// returns 0 when exactly on A -> B
// returns < 0 when on left side of A -> B
// returns the area of the triangle formed by A -> B -> P
// (also is its determinant aka cross product in two dimensions)
pim_inline float VEC_CALL sdEdge2D(float2 A, float2 B, float2 P)
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
    const float w = sdEdge2D(b, c, p) * rcpArea;
    const float u = sdEdge2D(c, a, p) * rcpArea;
    const float v = sdEdge2D(a, b, p) * rcpArea;
    return f3_v(w, u, v);
}

pim_inline float VEC_CALL sdTriangle2D(
    float2 a,
    float2 b,
    float2 c,
    float2 p)
{
    float2 ba = f2_sub(b, a);
    float2 cb = f2_sub(c, b);
    float2 ac = f2_sub(a, c);

    float2 pa = f2_sub(p, a);
    float2 pb = f2_sub(p, b);
    float2 pc = f2_sub(p, c);

    float2 pq0 = f2_sub(pa, f2_mulvs(ba, f1_saturate(f2_dot(pa, ba) / f2_lengthsq(ba))));
    float2 pq1 = f2_sub(pb, f2_mulvs(cb, f1_saturate(f2_dot(pb, cb) / f2_lengthsq(cb))));
    float2 pq2 = f2_sub(pc, f2_mulvs(ac, f1_saturate(f2_dot(pc, ac) / f2_lengthsq(ac))));

    float s = f1_sign(ba.x * ac.y - ba.y * ac.x);
    float2 t0 = f2_v(f2_lengthsq(pq0), s * (pa.x * ba.y - pa.y * ba.x));
    float2 t1 = f2_v(f2_lengthsq(pq1), s * (pb.x * cb.y - pb.y * cb.x));
    float2 t2 = f2_v(f2_lengthsq(pq2), s * (pc.x * ac.y - pc.y * ac.x));

    float2 d = f2_min(t0, f2_min(t1, t2));

    return -sqrtf(d.x) * f1_sign(d.y);
}

pim_inline float4 VEC_CALL isectTri3D(
    float3 ro, float3 rd,
    float3 A, float3 B, float3 C)
{
    const float e = 1.0f / (1 << 10);

    float3 BA = f3_sub(B, A);
    float3 CA = f3_sub(C, A);
    float3 P = f3_cross(rd, CA);
    float det = f3_dot(BA, P);
    if (det > e)
    {
        float rcpDet = 1.0f / det;
        float3 T = f3_sub(ro, A);
        float3 Q = f3_cross(T, BA);

        float t = f3_dot(CA, Q) * rcpDet;
        float u = f3_dot(T, P) * rcpDet;
        float v = f3_dot(rd, Q) * rcpDet;
        float w = 1.0f - u - v;

        return f4_v(w, u, v, t);
    }
    return f4_s(-1.0f);
}

pim_inline float VEC_CALL isectSphere3D(float3 ro, float3 rd, float3 c, float r)
{
    float3 roc = f3_sub(ro, c);
    float b = f3_dot(roc, rd);
    float s = f3_lengthsq(roc) - r * r;
    float h = b * b - s;
    if (h < 0.0f)
    {
        return -1.0f;
    }
    return -b - sqrtf(h);
}

pim_inline float VEC_CALL isectPlane3D(float3 ro, float3 rd, float4 plane)
{
    return -sdPlane3D(plane, ro) / f3_dot(rd, f3_v(plane.x, plane.y, plane.z));
}

PIM_C_END
