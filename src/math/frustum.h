#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/sdf.h"
#include "math/quat_funcs.h"
#include "math/float4x4_funcs.h"

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
    fwd.w = 1.0f;
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

pim_inline frus_t VEC_CALL frus_new(
    float4 e,       // eye point
    float4 r,       // right vector
    float4 u,       // up vector
    float4 f,       // forward vector
    float2 lo,      // minX, minY in [-1, 1]
    float2 hi,      // maxX, maxY in [-1, 1]
    float2 s,       // slope
    float2 nearFar) // clipping plane
{
    float x0 = lo.x;
    float x1 = hi.x;
    float y0 = lo.y;
    float y1 = hi.y;
    float z0 = nearFar.x;
    float z1 = nearFar.y;

    float4 p[8];
    for (i32 i = 0; i < 8; ++i)
    {
        float3 v;
        v.x = (i & 1) ? x1 : x0;
        v.z = (i & 2) ? z1 : z0;
        v.y = (i & 4) ? y1 : y0;
        p[i] = proj_pt(e, r, u, f, s, v);
    }

    frus_t frus;
    frus.x0 = triToPlane(p[4], p[6], p[0]);
    frus.x1 = triToPlane(p[1], p[3], p[5]);
    frus.y0 = triToPlane(p[0], p[2], p[1]);
    frus.y1 = triToPlane(p[5], p[7], p[4]);
    frus.z0 = triToPlane(p[1], p[5], p[0]);
    frus.z1 = triToPlane(p[7], p[3], p[6]);

    return frus;
}

pim_inline float VEC_CALL sdFrus(frus_t frus, float4 pt)
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

pim_inline float VEC_CALL sdFrusSph(frus_t frus, sphere_t sphere)
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

pim_inline float VEC_CALL sdFrusBoxSubplane(plane_t plane, box_t box)
{
    return sdPlane3D(plane, box.center) - f4_dot3(f4_abs(plane.value), box.extents);
}

pim_inline float VEC_CALL sdFrusBox(frus_t frus, box_t box)
{
    // factor out abs
    // box extents should always be in octant 0, but just in case.
    box.extents = f4_abs(box.extents);

    float a = sdFrusBoxSubplane(frus.x0, box);
    float b = sdFrusBoxSubplane(frus.x1, box);
    float g = f1_max(a, b);

    float c = sdFrusBoxSubplane(frus.y0, box);
    float d = sdFrusBoxSubplane(frus.y1, box);
    float h = f1_max(c, d);

    float e = sdFrusBoxSubplane(frus.z0, box);
    float f = sdFrusBoxSubplane(frus.z1, box);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

PIM_C_END
