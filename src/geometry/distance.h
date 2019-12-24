#pragma once

#define NOMINMAX

#include "geometry/types.h"
#include "math/vec_funcs.h"

inline float sdSphere(float3 p, float radius)
{
    using namespace math;

    return length(p) - radius;
}

inline float sdBox(float3 p, float3 b)
{
    using namespace math;

    float3 q = abs(p) - b;
    float l = length(max(q, float3(0.0f)));
    float r = min(0.0f, cmax(q));
    return l + r;
}

inline float sdCone(float3 p, float2 c)
{
    using namespace math;

    float q = length(float2(p.x, p.y));
    return dot(c, float2(q, p.z));
}

// signed
inline float sdPlane(float3 p, float3 n, float d)
{
    return math::dot(p, n) - d;
}

// unsigned
inline float udLine(float3 p, float3 a, float3 b)
{
    using namespace math;

    float3 pa = p - a;
    float3 ba = b - a;
    float h = dot(pa, ba) / lengthsq(ba);
    return length(pa - ba * h);
}

// unsigned
inline float udLineSegment(float3 p, float3 a, float3 b)
{
    using namespace math;

    float3 pa = p - a;
    float3 ba = b - a;
    float h = saturate(dot(pa, ba) / lengthsq(ba));
    return length(pa - ba * h);
}

// signed
inline float sdCapsule(float3 p, float3 a, float3 b, float r)
{
    return udLineSegment(p, a, b) - r;
}

// signed
inline float sdVerticalCapsule(float3 p, float h, float r)
{
    using namespace math;

    p.y -= clamp(p.y, 0.0f, h);
    return length(p) - r;
}

// unsigned
inline float udTriangle(float3 p, float3 a, float3 b, float3 c)
{
    using namespace math;

    float3 ba = b - a; float3 pa = p - a;
    float3 cb = c - b; float3 pb = p - b;
    float3 ac = a - c; float3 pc = p - c;
    float3 nor = cross(ba, ac);

    float r0 = sign(dot(cross(ba, nor), pa));
    float r1 = sign(dot(cross(cb, nor), pb));
    float r2 = sign(dot(cross(ac, nor), pc));
    float r3 = r0 + r1 + r2;

    if (r3 < 2.0f)
    {
        float r4 = lengthsq(ba * saturate(dot(ba, pa) / lengthsq(ba)) - pa);
        float r5 = lengthsq(cb * saturate(dot(cb, pb) / lengthsq(cb)) - pb);
        float r6 = lengthsq(ac * saturate(dot(ac, pc) / lengthsq(ac)) - pc);
        float r7 = min(r4, min(r5, r6));
        return sqrt(r7);
    }

    float r8 = lengthsq(distancesq(nor, pa)) / lengthsq(nor);
    return sqrt(r8);
}

// unsigned
inline float udQuad(float3 p, float3 a, float3 b, float3 c, float3 d)
{
    using namespace math;

    float3 ba = b - a; float3 pa = p - a;
    float3 cb = c - b; float3 pb = p - b;
    float3 dc = d - c; float3 pc = p - c;
    float3 ad = a - d; float3 pd = p - d;
    float3 nor = cross(ba, ad);

    float r0 = sign(dot(cross(ba, nor), pa));
    float r1 = sign(dot(cross(cb, nor), pb));
    float r2 = sign(dot(cross(dc, nor), pc));
    float r3 = sign(dot(cross(ad, nor), pd));
    float r4 = r0 + r1 + r2 + r3;

    if (r4 < 3.0f)
    {
        float r5 = lengthsq(ba * saturate(dot(ba, pa) / lengthsq(ba)) - pa);
        float r6 = lengthsq(cb * saturate(dot(cb, pb) / lengthsq(cb)) - pb);
        float r7 = lengthsq(dc * saturate(dot(dc, pc) / lengthsq(dc)) - pc);
        float r8 = lengthsq(ad * saturate(dot(ad, pd) / lengthsq(ad)) - pd);
        float r9 = min(r5, min(r6, min(r7, r8)));
        return sqrt(r9);
    }

    float r10 = lengthsq(distancesq(nor, pa)) / lengthsq(nor);
    return sqrt(r10);
}

// signed
inline float sdFrustum(float3 p, Frustum frustum)
{
    using namespace math;

    float dis = -float(0x7fffffff);
    for (Plane plane : frustum.planes)
    {
        dis = max(dis, sdPlane(p, plane.normal, plane.distance));
    }
    return dis;
}
