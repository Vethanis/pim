#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/float4_funcs.h"

pim_inline float VEC_CALL sdCircle(Circle c, float2 pt)
{
    return f2_distance(c.center, pt) - c.radius;
}

pim_inline float VEC_CALL sdSphere(Sphere sph, float4 pt)
{
    return f4_distance3(sph.value, pt) - sph.value.w;
}

pim_inline float VEC_CALL sdPlane2D(Plane2D plane, float2 pt)
{
    return f2_dot(plane.normal, pt) - plane.distance;
}

pim_inline Plane3D VEC_CALL plane_new(float4 dir, float4 pt)
{
    dir.w = f4_dot3(dir, pt);
    Plane3D y = { dir };
    return y;
}

pim_inline float VEC_CALL sdPlane3D(Plane3D plane, float4 pt)
{
    return f4_dot3(plane.value, pt) - plane.value.w;
}

pim_inline float VEC_CALL sdLine2D(Line2D line, float2 pt)
{
    float2 pa = f2_sub(pt, line.p0);
    float2 ba = f2_sub(line.p1, line.p0);
    float h = f1_saturate(f2_dot(pa, ba) / f2_lengthsq(ba));
    return f2_length(f2_sub(pa, f2_mulvs(ba, h)));
}

pim_inline float VEC_CALL sdLine3D(Capsule cap, float4 pt)
{
    float4 pa = f4_sub(pt, cap.p0r);
    float4 ba = f4_sub(cap.p1, cap.p0r);
    float h = f1_saturate(f4_dot3(pa, ba) / f4_lengthsq3(ba));
    return f4_length3(f4_sub(pa, f4_mulvs(ba, h))) - cap.p0r.w;
}

pim_inline float VEC_CALL sdBox2D(Box2D box, float2 pt)
{
    float2 center = f2_lerpvs(box.lo, box.hi, 0.5f);
    float2 extents = f2_sub(box.hi, center);
    float2 d = f2_sub(f2_abs(f2_sub(pt, center)), extents);
    return f2_length(f2_max(d, f2_0)) + f1_min(f2_hmax(d), 0.0f);
}

pim_inline float VEC_CALL sdBox3D(Box3D box, float4 pt)
{
    float4 center = f4_lerpvs(box.lo, box.hi, 0.5f);
    float4 extents = f4_sub(box.hi, center);
    float4 d = f4_sub(f4_abs(f4_sub(pt, center)), extents);
    return f4_length3(f4_max(d, f4_0)) + f1_min(f4_hmax3(d), 0.0f);
}

pim_inline float VEC_CALL sdPlaneCircle(Plane2D plane, Circle circle)
{
    return sdPlane2D(plane, circle.center) - circle.radius;
}

pim_inline float VEC_CALL sdPlaneSphere(Plane3D plane, Sphere sphere)
{
    return sdPlane3D(plane, sphere.value) - sphere.value.w;
}

pim_inline float VEC_CALL sdPlaneBox2D(Plane2D plane, Box2D box)
{
    float2 center = f2_lerpvs(box.lo, box.hi, 0.5f);
    float2 extents = f2_sub(box.hi, center);
    return sdPlane2D(plane, center) - f2_dot(f2_abs(plane.normal), f2_abs(extents));
}

pim_inline float VEC_CALL sdPlaneBox3D(Plane3D plane, Box3D box)
{
    float4 center = f4_lerpvs(box.lo, box.hi, 0.5f);
    float4 extents = f4_sub(box.hi, center);
    return sdPlane3D(plane, center) - f4_dot3(f4_abs(plane.value), f4_abs(extents));
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
pim_inline float4 VEC_CALL bary2D(
    float2 a,
    float2 b,
    float2 c,
    float rcpArea,
    float2 p)
{
    float w = sdEdge2D(b, c, p) * rcpArea;
    float u = sdEdge2D(c, a, p) * rcpArea;
    float v = sdEdge2D(a, b, p) * rcpArea;
    return f4_v(w, u, v, 0.0f);
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

    float h0 = f1_saturate(f2_dot(pa, ba) / f2_dot(ba, ba));
    float h1 = f1_saturate(f2_dot(pb, cb) / f2_dot(cb, cb));
    float h2 = f1_saturate(f2_dot(pc, ac) / f2_dot(ac, ac));

    float2 pq0 = f2_sub(pa, f2_mulvs(ba, h0));
    float2 pq1 = f2_sub(pb, f2_mulvs(cb, h1));
    float2 pq2 = f2_sub(pc, f2_mulvs(ac, h2));

    float s = f1_sign(ba.x * ac.y - ba.y * ac.x);
    float2 t0 = f2_v(f2_dot(pq0, pq0), s * (pa.x * ba.y - pa.y * ba.x));
    float2 t1 = f2_v(f2_dot(pq1, pq1), s * (pb.x * cb.y - pb.y * cb.x));
    float2 t2 = f2_v(f2_dot(pq2, pq2), s * (pc.x * ac.y - pc.y * ac.x));

    float2 d = f2_min(t0, f2_min(t1, t2));

    return -sqrtf(d.x) * f1_sign(d.y);
}

pim_inline float VEC_CALL sdTriangle3D(
    float4 a,
    float4 b,
    float4 c,
    float4 p)
{
    float4 ba = f4_sub(b, a); float4 pa = f4_sub(p, a);
    float4 cb = f4_sub(c, b); float4 pb = f4_sub(p, b);
    float4 ac = f4_sub(a, c); float4 pc = f4_sub(p, c);
    float4 nor = f4_cross3(ba, ac);

    float t0 = f1_sign(f4_dot3(f4_cross3(ba, nor), pa));
    float t1 = f1_sign(f4_dot3(f4_cross3(cb, nor), pb));
    float t2 = f1_sign(f4_dot3(f4_cross3(ac, nor), pc));
    float t;
    if ((t0 + t1 + t2) < 2.0f)
    {
        t0 = f1_sat(f4_dot3(ba, pa) / f4_lengthsq3(ba));
        t1 = f1_sat(f4_dot3(cb, pb) / f4_lengthsq3(cb));
        t2 = f1_sat(f4_dot3(ac, pc) / f4_lengthsq3(ac));
        t0 = f4_lengthsq3(f4_sub(f4_mulvs(ba, t0), pa));
        t1 = f4_lengthsq3(f4_sub(f4_mulvs(cb, t1), pb));
        t2 = f4_lengthsq3(f4_sub(f4_mulvs(ac, t2), pc));
        t = f1_min(t0, f1_min(t1, t2));
    }
    else
    {
        t0 = f4_dot3(nor, pa);
        t = t0 * t0 / f4_lengthsq3(nor);
    }
    return sqrtf(t);
}

pim_inline float4 VEC_CALL isectTri3D(Ray ray, float4 A, float4 B, float4 C)
{
    float4 BA = f4_sub(B, A);
    float4 CA = f4_sub(C, A);
    float4 P = f4_cross3(ray.rd, CA);
    float det = f4_dot3(BA, P);
    if (det > kEpsilon)
    {
        float rcpDet = 1.0f / det;
        float4 T = f4_sub(ray.ro, A);
        float4 Q = f4_cross3(T, BA);

        float t = f4_dot3(CA, Q) * rcpDet;
        float u = f4_dot3(T, P) * rcpDet;
        float v = f4_dot3(ray.rd, Q) * rcpDet;
        float w = 1.0f - u - v;

        return f4_v(w, u, v, t);
    }
    return f4_s(-1.0f);
}

// returns [near, far] intersection distances
// if near > far, the ray does not intersect
pim_inline float2 VEC_CALL isectSphere3D(float4 ro, float4 rd, Sphere sph)
{
    // move to sphere local space
    ro = f4_sub(ro, sph.value);
    float radius = sph.value.w;
    float b = 2.0f * f4_dot3(rd, ro);
    float c = f4_dot3(ro, ro) - radius * radius;
    float d = (b * b) - 4.0f * c;
    if (d <= 0.0f)
    {
        return f2_v(1.0f, -1.0f);
    }
    float nb = -b;
    float sqrtd = sqrtf(d);
    return f2_v((nb - sqrtd) * 0.5f, (nb + sqrtd) * 0.5f);
}

pim_inline float VEC_CALL isectPlane3D(Ray ray, Plane3D plane)
{
    return -sdPlane3D(plane, ray.ro) / f4_dot3(ray.rd, plane.value);
}

// returns [near, far] intersection distances
// if near > far, the ray does not intersect
pim_inline float2 VEC_CALL isectBox3D(
    float4 ro,      // ray origin
    float4 rcpRd,   // 1 / ray direction
    Box3D box)
{
    float4 tx1 = f4_mul(f4_sub(box.lo, ro), rcpRd);
    float4 tx2 = f4_mul(f4_sub(box.hi, ro), rcpRd);
    float tmin = f4_hmax3(f4_min(tx1, tx2));
    float tmax = f4_hmin3(f4_max(tx1, tx2));
    return f2_v(tmin, tmax);
}

pim_inline Plane3D VEC_CALL triToPlane(float4 A, float4 B, float4 C)
{
    Plane3D pl;
    pl.value = f4_normalize3(f4_cross3(f4_sub(B, A), f4_sub(C, A)));
    pl.value.w = f4_dot3(pl.value, A);
    return pl;
}

pim_inline Sphere VEC_CALL triToSphere(float4 A, float4 B, float4 C)
{
    float4 center = f4_divvs(f4_add(A, f4_add(B, C)), 3.0f);
    float da = f4_distancesq3(A, center);
    float db = f4_distancesq3(B, center);
    float dc = f4_distancesq3(C, center);
    float rsqr = f1_max(da, f1_max(db, dc));
    center.w = sqrtf(rsqr);
    Sphere sph = { center };
    return sph;
}

pim_inline Box3D VEC_CALL triToBox(float4 A, float4 B, float4 C)
{
    float4 lo = f4_min(A, f4_min(B, C));
    float4 hi = f4_max(A, f4_max(B, C));
    float4 center = f4_lerpvs(lo, hi, 0.5f);
    float4 extents = f4_sub(hi, center);
    Box3D box = { center, extents };
    return box;
}

pim_inline Sphere VEC_CALL sphTransform(Sphere sph, float4 translation, float4 scale)
{
    float r = sph.value.w;
    sph.value = f4_add(sph.value, translation);
    float s = f4_hmax3(f4_abs(scale));
    sph.value.w = r * s;
    return sph;
}

PIM_C_END
