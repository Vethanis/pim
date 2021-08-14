#pragma once

#include "math/types.h"
#include "math/sdf.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"

PIM_C_BEGIN

// returns value between zNear and zFar
// t is in [0, 1] range
// http://advances.realtimerendering.com/s2016/Siggraph2016_idTech6.pdf
pim_inline float VEC_CALL LerpZ(float zNear, float zFar, float t)
{
    return zNear * powf(zFar / zNear, t);
}

// returns value between 0 and 1
// z is in [zNear, zFar] range
// http://advances.realtimerendering.com/s2016/Siggraph2016_idTech6.pdf
pim_inline float VEC_CALL UnlerpZ(float zNear, float zFar, float z)
{
    return log2f(z / zNear) / log2f(zFar / zNear);
}

pim_inline float2 VEC_CALL proj_slope(float fovy, float aspect)
{
    float tanHalfFov = tanf(fovy * 0.5f);
    float2 slope = { aspect * tanHalfFov, tanHalfFov };
    return slope;
}

pim_inline float4 VEC_CALL proj_dir(
    float4 right,
    float4 up,
    float4 fwd,
    float2 slope,
    float2 coord)
{
    coord = f2_mul(coord, slope);
    right = f4_mulvs(right, coord.x);
    up = f4_mulvs(up, coord.y);
    fwd = f4_add(fwd, right);
    fwd = f4_add(fwd, up);
    fwd = f4_normalize3(fwd);
    return fwd;
}

pim_inline float4 VEC_CALL proj_pt(
    float4 eye,
    float4 right,
    float4 up,
    float4 fwd,
    float2 slope,
    float3 coord)
{
    float4 rd = proj_dir(right, up, fwd, slope, f2_v(coord.x, coord.y));
    return f4_add(eye, f4_mulvs(rd, coord.z));
}

pim_inline float2 VEC_CALL unproj_dir(
    float4 right,
    float4 up,
    float4 fwd,
    float2 slope,
    float4 rd)
{
    float r = f4_dot3(rd, right) / slope.x;
    float u = f4_dot3(rd, up) / slope.y;
    float rcpf = 1.0f / f4_dot3(rd, fwd);
    return f2_v(r * rcpf, u * rcpf);
}

pim_inline float3 VEC_CALL unproj_pt(
    float4 eye,
    float4 right,
    float4 up,
    float4 fwd,
    float2 slope,
    float4 pt)
{
    float4 rd = f4_sub(pt, eye);
    float distance = f4_length3(rd);
    rd = f4_divvs(rd, distance);
    float z = distance * f1_sign(f4_dot3(rd, fwd));
    float2 xy = unproj_dir(right, up, fwd, slope, rd);
    return f3_v(xy.x, xy.y, z);
}

pim_inline Frustum VEC_CALL frus_new(
    float4 e,       // eye point
    float4 r,       // right vector
    float4 u,       // up vector
    float4 f,       // forward vector
    float2 lo,      // minX, minY in [-1, 1]
    float2 hi,      // maxX, maxY in [-1, 1]
    float2 s,       // slope
    float zNear,
    float zFar) // clipping plane
{
    float x0 = lo.x;
    float x1 = hi.x;
    float y0 = lo.y;
    float y1 = hi.y;
    float z0 = zNear;
    float z1 = zFar;

    float4 p[8];
    for (i32 i = 0; i < 8; ++i)
    {
        float3 v;
        v.x = (i & 1) ? x1 : x0;
        v.z = (i & 2) ? z1 : z0;
        v.y = (i & 4) ? y1 : y0;
        p[i] = proj_pt(e, r, u, f, s, v);
    }

    Frustum frus;
    frus.x0 = triToPlane(p[4], p[6], p[0]);
    frus.x1 = triToPlane(p[1], p[3], p[5]);
    frus.y0 = triToPlane(p[0], p[2], p[1]);
    frus.y1 = triToPlane(p[5], p[7], p[4]);
    frus.z0 = triToPlane(p[1], p[5], p[0]);
    frus.z1 = triToPlane(p[7], p[3], p[6]);

    return frus;
}

pim_inline float VEC_CALL sdFrus(Frustum frus, float4 pt)
{
    float a = sdPlane3D(frus.x0, pt);
    float b = sdPlane3D(frus.x1, pt);
    float g = f1_max(a, b);

    float c = sdPlane3D(frus.y0, pt);
    float d = sdPlane3D(frus.y1, pt);
    float h = f1_max(c, d);

    float e = sdPlane3D(frus.z0, pt);
    float f = sdPlane3D(frus.z1, pt);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

pim_inline float VEC_CALL sdFrusSph(Frustum frus, Sphere sphere)
{
    float a = sdPlaneSphere(frus.x0, sphere);
    float b = sdPlaneSphere(frus.x1, sphere);
    float g = f1_max(a, b);

    float c = sdPlaneSphere(frus.y0, sphere);
    float d = sdPlaneSphere(frus.y1, sphere);
    float h = f1_max(c, d);

    float e = sdPlaneSphere(frus.z0, sphere);
    float f = sdPlaneSphere(frus.z1, sphere);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

pim_inline float VEC_CALL sdFrusBoxSubplane(Plane3D plane, float4 center, float4 extents)
{
    return sdPlane3D(plane, center) - f4_dot3(f4_abs(plane.value), extents);
}

pim_inline float VEC_CALL sdFrusBox(Frustum frus, Box3D box)
{
    // factor out abs
    // box extents should always be in octant 0, but just in case.
    float4 center = f4_lerpvs(box.lo, box.hi, 0.5f);
    float4 extents = f4_abs(f4_sub(box.hi, center));

    float a = sdFrusBoxSubplane(frus.x0, center, extents);
    float b = sdFrusBoxSubplane(frus.x1, center, extents);
    float g = f1_max(a, b);

    float c = sdFrusBoxSubplane(frus.y0, center, extents);
    float d = sdFrusBoxSubplane(frus.y1, center, extents);
    float h = f1_max(c, d);

    float e = sdFrusBoxSubplane(frus.z0, center, extents);
    float f = sdFrusBoxSubplane(frus.z1, center, extents);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

PIM_C_END
