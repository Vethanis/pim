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

    float4 lN = f4_normalize3(f4_cross3(f4_sub(LTN, LBN), f4_sub(LBF, LBN)));
    float lD = f4_dot3(lN, LBN);

    float4 rN = f4_normalize3(f4_cross3(f4_sub(RTF, RBF), f4_sub(RBN, RBF)));
    float rD = f4_dot3(rN, RBF);

    float4 tN = f4_normalize3(f4_cross3(f4_sub(RTF, RTN), f4_sub(LTN, RTN)));
    float tD = f4_dot3(tN, RTN);

    float4 bN = f4_normalize3(f4_cross3(f4_sub(LBF, LBN), f4_sub(RBN, LBN)));
    float bD = f4_dot3(bN, LBN);

    float4 nN = f4_normalize3(f4_cross3(f4_sub(RTN, RBN), f4_sub(LBN, RBN)));
    float nD = f4_dot3(nN, RBN);

    float4 fN = f4_normalize3(f4_cross3(f4_sub(LTF, LBF), f4_sub(RBF, LBF)));
    float fD = f4_dot3(fN, LBF);

    frus_t frus =
    {
        { lN.x, lN.y, lN.z, lD },
        { rN.x, rN.y, rN.z, rD },
        { bN.x, bN.y, bN.z, bD },
        { tN.x, tN.y, tN.z, tD },
        { nN.x, nN.y, nN.z, nD },
        { fN.x, fN.y, fN.z, fD },
    };
    return frus;
}

pim_inline float VEC_CALL sdFrus(frus_t frus, float4 pt)
{
    float a = sdPlane3D(frus.x0, pt);
    float b = sdPlane3D(frus.x1, pt);
    float c = sdPlane3D(frus.y0, pt);
    float d = sdPlane3D(frus.y1, pt);
    float e = sdPlane3D(frus.z0, pt);
    float f = sdPlane3D(frus.z1, pt);

    float g = f1_max(a, b);
    float h = f1_max(c, d);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

pim_inline float VEC_CALL sdFrusSph(frus_t frus, float4 sphere)
{
    float a = sdPlaneSphere(frus.x0, sphere);
    float b = sdPlaneSphere(frus.x1, sphere);
    float c = sdPlaneSphere(frus.y0, sphere);
    float d = sdPlaneSphere(frus.y1, sphere);
    float e = sdPlaneSphere(frus.z0, sphere);
    float f = sdPlaneSphere(frus.z1, sphere);

    float g = f1_max(a, b);
    float h = f1_max(c, d);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

pim_inline float VEC_CALL sdFrusBoxTest(frus_t frus, box_t box)
{
    float a = sdPlaneBox3D(frus.x0, box.center, box.extents);
    if (a > 0.0f)
    {
        return a;
    }
    float b = sdPlaneBox3D(frus.x1, box.center, box.extents);
    if (b > 0.0f)
    {
        return b;
    }
    float c = sdPlaneBox3D(frus.y0, box.center, box.extents);
    if (c > 0.0f)
    {
        return c;
    }
    float d = sdPlaneBox3D(frus.y1, box.center, box.extents);
    if (d > 0.0f)
    {
        return d;
    }
    float e = sdPlaneBox3D(frus.z0, box.center, box.extents);
    if (e > 0.0f)
    {
        return e;
    }
    float f = sdPlaneBox3D(frus.z1, box.center, box.extents);
    if (f > 0.0f)
    {
        return f;
    }
    return -1.0f;
}

pim_inline float4 VEC_CALL triToSphere(float4 A, float4 B, float4 C)
{
    float4 center = f4_mulvs(f4_add(A, f4_add(B, C)), 0.33333333f);
    float da = f4_distancesq3(A, center);
    float db = f4_distancesq3(B, center);
    float dc = f4_distancesq3(C, center);
    float dsq = f1_max(da, f1_max(db, dc));
    center.w = sqrtf(dsq);
    return center;
}

pim_inline box_t VEC_CALL triToBox(float4 A, float4 B, float4 C)
{
    float4 lo = f4_min(A, f4_min(B, C));
    float4 hi = f4_max(A, f4_max(B, C));
    float4 center = f4_lerp(lo, hi, 0.5f);
    float4 extents = f4_sub(hi, center);
    box_t box = { center, extents };
    return box;
}

pim_inline box_t VEC_CALL TransformBox(float4x4 M, box_t x)
{
    box_t y;

    float4 a = f4_abs(f4_mulvs(M.c0, x.extents.x));
    float4 b = f4_abs(f4_mulvs(M.c1, x.extents.y));
    float4 c = f4_abs(f4_mulvs(M.c2, x.extents.z));
    y.extents = f4_add(f4_add(a, b), c);

    y.center = f4x4_mul_pt(M, x.center);
    return y;
}

PIM_C_END
