#pragma once

#include "common/macro.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"

PIM_C_BEGIN

pim_inline float VEC_CALL SphereArea(float radius)
{
    return 4.0f * kPi * (radius * radius);
}

pim_inline float VEC_CALL DiskArea(float radius)
{
    return kPi * (radius * radius);
}

pim_inline float VEC_CALL TubeArea(float radius, float width)
{
    return (2.0f * kPi * radius * width) + (4.0f * kPi * radius * radius);
}

pim_inline float VEC_CALL RectArea(float width, float height)
{
    return width * height;
}

pim_inline float VEC_CALL TriArea3D(float4 A, float4 B, float4 C)
{
    return 0.5f * f4_length3(f4_cross3(f4_sub(B, A), f4_sub(C, A)));
}

pim_inline float VEC_CALL TriArea2D(Tri2D tri)
{
    float2 ab = f2_sub(tri.b, tri.a);
    float2 ac = f2_sub(tri.c, tri.a);
    float cr = ab.x * ac.y - ac.x * ab.y;
    return 0.5f * cr;
}

PIM_C_END
