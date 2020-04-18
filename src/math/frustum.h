#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/types.h"
#include "math/sdf.h"
#include "math/quat_funcs.h"

// outward facing plane normals and positive distances
typedef struct frus_s
{
    float4 x0;
    float4 x1;
    float4 y0;
    float4 y1;
    float4 z0;
    float4 z1;
} frus_t;

typedef struct box_s
{
    float3 center;
    float3 extents;
} box_t;

pim_inline float2 VEC_CALL proj_slope(float fovy, float aspect)
{
    float tanHalfFov = tanf(fovy * 0.5f);
    float2 slope = { aspect * tanHalfFov, tanHalfFov };
    return slope;
}

pim_inline float3 VEC_CALL proj_dir(
    float3 right,
    float3 up,
    float3 fwd,
    float2 slope,
    float2 coord)
{
    coord = f2_mul(coord, slope);
    right = f3_mulvs(right, coord.x);
    up = f3_mulvs(up, coord.y);
    fwd = f3_add(fwd, right);
    fwd = f3_add(fwd, up);
    return f3_normalize(fwd);
}

pim_inline float3 VEC_CALL proj_pt(
    float3 eye,
    float3 right,
    float3 up,
    float3 fwd,
    float2 slope,
    float3 coord)
{
    float3 rd = proj_dir(right, up, fwd, slope, f2_v(coord.x, coord.y));
    return f3_add(eye, f3_mulvs(rd, coord.z));
}

pim_inline frus_t VEC_CALL frus_new(
    float3 eye,     // frustum origin
    float3 right,
    float3 up,
    float3 fwd,
    float2 lo,      // minX, minY in [-1, 1]
    float2 hi,      // maxX, maxY in [-1, 1]
    float2 slope,
    float2 nearFar) // clipping plane
{
    float3 LBN = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, lo.y, nearFar.x));
    float3 LBF = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, lo.y, nearFar.y));
    float3 LTN = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, hi.y, nearFar.x));
    float3 LTF = proj_pt(eye, right, up, fwd, slope, f3_v(lo.x, hi.y, nearFar.y));
    float3 RBN = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, lo.y, nearFar.x));
    float3 RBF = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, lo.y, nearFar.y));
    float3 RTN = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, hi.y, nearFar.x));
    float3 RTF = proj_pt(eye, right, up, fwd, slope, f3_v(hi.x, hi.y, nearFar.y));

    float3 lN = f3_normalize(f3_cross(f3_sub(LTN, LBN), f3_sub(LBF, LBN)));
    float lD = f3_dot(lN, LBN);

    float3 rN = f3_normalize(f3_cross(f3_sub(RTF, RBF), f3_sub(RBN, RBF)));
    float rD = f3_dot(rN, RBF);

    float3 tN = f3_normalize(f3_cross(f3_sub(RTF, RTN), f3_sub(LTN, RTN)));
    float tD = f3_dot(tN, RTN);

    float3 bN = f3_normalize(f3_cross(f3_sub(LBF, LBN), f3_sub(RBN, LBN)));
    float bD = f3_dot(bN, LBN);

    float3 nN = f3_normalize(f3_cross(f3_sub(RTN, RBN), f3_sub(LBN, RBN)));
    float nD = f3_dot(nN, RBN);

    float3 fN = f3_normalize(f3_cross(f3_sub(LTF, LBF), f3_sub(RBF, LBF)));
    float fD = f3_dot(fN, LBF);

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

pim_inline float VEC_CALL sdFrus(frus_t frus, float3 pt)
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

pim_inline float VEC_CALL sdFrusBox(frus_t frus, box_t box)
{
    float a = sdPlaneBox3D(frus.x0, box.center, box.extents);
    float b = sdPlaneBox3D(frus.x1, box.center, box.extents);
    float c = sdPlaneBox3D(frus.y0, box.center, box.extents);
    float d = sdPlaneBox3D(frus.y1, box.center, box.extents);
    float e = sdPlaneBox3D(frus.z0, box.center, box.extents);
    float f = sdPlaneBox3D(frus.z1, box.center, box.extents);

    float g = f1_max(a, b);
    float h = f1_max(c, d);
    float i = f1_max(e, f);

    return f1_max(g, f1_max(h, i));
}

pim_inline float4 VEC_CALL triToSphere(float3 A, float3 B, float3 C)
{
    float3 center = f3_mulvs(f3_add(A, f3_add(B, C)), 0.33333333f);
    float da = f3_distancesq(A, center);
    float db = f3_distancesq(B, center);
    float dc = f3_distancesq(C, center);
    float dsq = f1_max(da, f1_max(db, dc));
    float radius = sqrtf(dsq);
    return f3_f4(center, radius);
}

pim_inline box_t VEC_CALL triToBox(float3 A, float3 B, float3 C)
{
    float3 lo = f3_min(A, f3_min(B, C));
    float3 hi = f3_max(A, f3_max(B, C));
    float3 center = f3_mulvs(f3_add(hi, lo), 0.5f);
    float3 extents = f3_sub(hi, center);
    box_t box = { center, extents };
    return box;
}

PIM_C_END
