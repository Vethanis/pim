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
    float4 eye,     // frustum origin
    float4 right,
    float4 up,
    float4 fwd,
    float2 lo,      // minX, minY in [-1, 1]
    float2 hi,      // maxX, maxY in [-1, 1]
    float2 slope,
    float2 nearFar) // clipping plane
{
    float4 LBN = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, lo.y, nearFar.x));
    float4 LBF = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, lo.y, nearFar.y));
    float4 LTN = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, hi.y, nearFar.x));
    float4 LTF = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, hi.y, nearFar.y));
    float4 RBN = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, lo.y, nearFar.x));
    float4 RBF = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, lo.y, nearFar.y));
    float4 RTN = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, hi.y, nearFar.x));
    float4 RTF = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, hi.y, nearFar.y));

    frus_t frus;
    frus.x0 = triToPlane(LBN, LTN, LBF);
    frus.x1 = triToPlane(RBF, RTF, RBN);
    frus.y0 = triToPlane(LBN, LBF, RBN);
    frus.y1 = triToPlane(RTN, RTF, LTN);
    frus.z0 = triToPlane(RBN, RTN, LBN);
    frus.z1 = triToPlane(LBF, LTF, RBF);

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
