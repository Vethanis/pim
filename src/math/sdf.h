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

pim_inline float VEC_CALL sdSphere(float4 sph, float4 pt)
{
    return f4_distance3(sph, pt) - sph.w;
}

pim_inline float VEC_CALL sdPlane2D(float2 n, float d, float2 pt)
{
    return f2_dot(n, pt) - d;
}

pim_inline float VEC_CALL sdPlane3D(float4 plane, float4 pt)
{
    return f4_dot3(plane, pt) - plane.w;
}

pim_inline float VEC_CALL sdLine2D(float2 a, float2 b, float2 pt)
{
    float2 pa = f2_sub(pt, a);
    float2 ba = f2_sub(b, a);
    float h = f1_saturate(f2_dot(pa, ba) / f2_lengthsq(ba));
    return f2_length(f2_sub(pa, f2_mulvs(ba, h)));
}

pim_inline float VEC_CALL sdLine3D(float4 a, float4 b, float r, float4 pt)
{
    float4 pa = f4_sub(pt, a);
    float4 ba = f4_sub(b, a);
    float h = f1_saturate(f4_dot3(pa, ba) / f4_lengthsq(ba));
    return f4_length3(f4_sub(pa, f4_mulvs(ba, h))) - r;
}

pim_inline float VEC_CALL sdBox2D(float2 c, float2 e, float2 pt)
{
    float2 d = f2_sub(f2_abs(f2_sub(pt, c)), e);
    return f2_length(f2_max(d, f2_0)) + f1_min(f2_hmax(d), 0.0f);
}

pim_inline float VEC_CALL sdBox3D(float4 c, float4 e, float4 pt)
{
    float4 d = f4_sub(f4_abs(f4_sub(pt, c)), e);
    return f4_length3(f4_max(d, f4_0)) + f1_min(f4_hmax3(d), 0.0f);
}

pim_inline float VEC_CALL sdPlaneCircle(float2 n, float d, float2 c, float r)
{
    return sdPlane2D(n, d, c) - r;
}

pim_inline float VEC_CALL sdPlaneSphere(float4 plane, float4 sphere)
{
    return sdPlane3D(plane, sphere) - sphere.w;
}

pim_inline float VEC_CALL sdPlaneBox2D(float2 n, float d, float2 c, float2 e)
{
    return sdPlane2D(n, d, c) - f2_dot(f2_abs(n), f2_abs(e));
}

pim_inline float VEC_CALL sdPlaneBox3D(float4 plane, float4 c, float4 e)
{
    return sdPlane3D(plane, c) - f4_dot3(f4_abs(plane), f4_abs(e));
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
pim_inline float3 VEC_CALL bary2D(
    float2 a,
    float2 b,
    float2 c,
    float rcpArea,
    float2 p)
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
    float4 ro, float4 rd,
    float4 A, float4 B, float4 C)
{
    const float e = 1.0f / (1 << 10);

    float4 BA = f4_sub(B, A);
    float4 CA = f4_sub(C, A);
    float4 P = f4_cross3(rd, CA);
    float det = f4_dot3(BA, P);
    if (det > e)
    {
        float rcpDet = 1.0f / det;
        float4 T = f4_sub(ro, A);
        float4 Q = f4_cross3(T, BA);

        float t = f4_dot3(CA, Q) * rcpDet;
        float u = f4_dot3(T, P) * rcpDet;
        float v = f4_dot3(rd, Q) * rcpDet;
        float w = 1.0f - u - v;

        return f4_v(w, u, v, t);
    }
    return f4_s(-1.0f);
}

pim_inline float VEC_CALL isectSphere3D(float4 ro, float4 rd, float4 c, float r)
{
    float4 roc = f4_sub(ro, c);
    float b = f4_dot3(roc, rd);
    float s = f4_lengthsq3(roc) - r * r;
    float h = b * b - s;
    if (h < 0.0f)
    {
        return -1.0f;
    }
    return -b - sqrtf(h);
}

pim_inline float VEC_CALL isectPlane3D(float4 ro, float4 rd, float4 plane)
{
    return -sdPlane3D(plane, ro) / f4_dot3(rd, plane);
}

// returns [near, far] intersection distances
// if far < near, the ray does not intersect the box
pim_inline float2 VEC_CALL isectBox3D(
    float4 ro,      // ray origin
    float4 rcpRd,   // 1 / ray direction
    box_t box)
{
    float4 lo = f4_sub(box.center, box.extents);
    float4 hi = f4_add(box.center, box.extents);
    float4 tx1 = f4_mul(f4_sub(lo, ro), rcpRd);
    float4 tx2 = f4_mul(f4_sub(hi, ro), rcpRd);
    float tmin = f4_hmax3(f4_min(tx1, tx2));
    float tmax = f4_hmin3(f4_max(tx1, tx2));
    return f2_v(tmin, tmax);
}

PIM_C_END
