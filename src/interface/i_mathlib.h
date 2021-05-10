#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h

#include "interface/i_types.h"
#include <math.h>

#ifndef M_PI
#   define M_PI 3.14159265358979323846f
#endif

#ifndef IS_NAN
#   define IS_NAN(x) ( isnan(x) )
#endif

// These are inlined and vectorcalled for performance sake

pim_inline float VEC_CALL DotProduct(const vec3_t a, const vec3_t b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

pim_inline void VEC_CALL VectorSubtract(const vec3_t a, const vec3_t b, vec3_t c)
{
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
}

pim_inline void VEC_CALL VectorAdd(const vec3_t a, const vec3_t b, vec3_t c)
{
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
}

pim_inline void VEC_CALL VectorCopy(const vec3_t a, vec3_t b)
{
    b[0] = a[0]; b[1] = a[1]; b[2] = a[2];
}

pim_inline void VEC_CALL VectorMA(
    const vec3_t a,
    float s,
    const vec3_t b,
    vec3_t c)
{
    c[0] = a[0] + s * b[0];
    c[1] = a[1] + s * b[1];
    c[2] = a[2] + s * b[2];
}

pim_inline i32 VEC_CALL VectorCompare(const vec3_t a, const vec3_t b)
{
    return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]);
}

pim_inline float VEC_CALL Length(const vec3_t v)
{
    float l2 = DotProduct(v, v);
    return (l2 > Q_EPSILON) ? sqrtf(l2) : 0.0f;
}

pim_inline void VEC_CALL CrossProduct(
    const vec3_t v1,
    const vec3_t v2,
    vec3_t cross)
{
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

pim_inline float VEC_CALL VectorNormalize(vec3_t v)
{
    float l2 = DotProduct(v, v);
    if (l2 > Q_EPSILON)
    {
        float l = sqrtf(l2);
        float rcpLen = 1.0f / l;
        v[0] *= rcpLen;
        v[1] *= rcpLen;
        v[2] *= rcpLen;
        return l;
    }
    return 0.0f;
}

pim_inline void VEC_CALL VectorInverse(vec3_t v)
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

pim_inline void VEC_CALL VectorScale(vec3_t a, vec_t s, vec3_t c)
{
    c[0] = a[0] * s;
    c[1] = a[1] * s;
    c[2] = a[2] * s;
}

pim_inline i32 VEC_CALL Q_log2(i32 val)
{
    i32 answer = 0;
    while (val >>= 1)
    {
        answer++;
    }
    return answer;
}

pim_inline void VEC_CALL R_ConcatRotations(
    float const in1[3][3],
    float const in2[3][3],
    float out[3][3])
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];

    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];

    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

pim_inline void VEC_CALL R_ConcatTransforms(
    float const in1[3][4],
    float const in2[3][4],
    float out[3][4])
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
    out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];

    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
    out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];

    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
    out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];
}

pim_inline i32 VEC_CALL GreatestCommonDivisor(i32 i1, i32 i2)
{
    if (i1 > i2)
    {
        return i2 ? GreatestCommonDivisor(i2, i1 % i2) : i1;
    }
    else
    {
        return i1 ? GreatestCommonDivisor(i1, i2 % i1) : i2;
    }
}

pim_inline void VEC_CALL AngleVectors(
    const vec3_t angles,
    vec3_t forward,
    vec3_t right,
    vec3_t up)
{
    float angle = angles[YAW] * (M_PI * 2.0f / 360.0f);
    float sy = sinf(angle);
    float cy = cosf(angle);
    angle = angles[PITCH] * (M_PI * 2.0f / 360.0f);
    float sp = sinf(angle);
    float cp = cosf(angle);
    angle = angles[ROLL] * (M_PI * 2.0f / 360.0f);
    float sr = sinf(angle);
    float cr = cosf(angle);

    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
    right[0] = -1.0f * sr * sp * cy + -1.0f * cr * -sy;
    right[1] = -1.0f * sr * sp * sy + -1.0f * cr * cy;
    right[2] = -1.0f * sr * cp;
    up[0] = cr * sp * cy + -sr * -sy;
    up[1] = cr * sp * sy + -sr * cy;
    up[2] = cr * cp;
}

#define BOX_ON_PLANE_SIDE(...) BoxOnPlaneSide(__VA_ARGS__)
pim_inline i32 VEC_CALL BoxOnPlaneSide(
    const vec3_t emins,
    const vec3_t emaxs,
    const mplane_t *p)
{
    i32 type = p->type;
    float pdist = p->dist;
    if (type < 3)
    {
        float axisLo = emins[type];
        float axisHi = emaxs[type];
        return (pdist <= axisLo) ? (1) : ((pdist >= axisHi) ? (2) : (3));
    }
    float dist1 = 0.0f;
    float dist2 = 0.0f;
    i32 sides = 0;
    switch (p->signbits)
    {
    default:
        break;
    case 0:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 1:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        break;
    case 2:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 3:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        break;
    case 4:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 5:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
        break;
    case 6:
        dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    case 7:
        dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
        dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
        break;
    }

    sides |= (dist1 >= pdist) * 1;
    sides |= (dist2 < pdist) * 2;

    return sides;
}

pim_inline float VEC_CALL anglemod(float a)
{
    a = (360.0f / 65536.0f) * ((i32)(a * (65536.0f / 360.0f)) & 0xffff);
    return a;
}
