#pragma once

#include "math/types.h"
#include "math/float4_funcs.h"

pim_inline float4 VEC_CALL operator + (float4 lhs, float4 rhs)
{
    return f4_add(lhs, rhs);
}
pim_inline float4 VEC_CALL operator + (float lhs, float4 rhs)
{
    return f4_addsv(lhs, rhs);
}
pim_inline float4 VEC_CALL operator + (float4 lhs, float rhs)
{
    return f4_addvs(lhs, rhs);
}

pim_inline float4 VEC_CALL operator - (float4 lhs, float4 rhs)
{
    return f4_sub(lhs, rhs);
}
pim_inline float4 VEC_CALL operator - (float lhs, float4 rhs)
{
    return f4_subsv(lhs, rhs);
}
pim_inline float4 VEC_CALL operator - (float4 lhs, float rhs)
{
    return f4_subvs(lhs, rhs);
}

pim_inline float4 VEC_CALL operator * (float4 lhs, float4 rhs)
{
    return f4_mul(lhs, rhs);
}
pim_inline float4 VEC_CALL operator * (float lhs, float4 rhs)
{
    return f4_mulvs(rhs, lhs);
}
pim_inline float4 VEC_CALL operator * (float4 lhs, float rhs)
{
    return f4_mulvs(lhs, rhs);
}

pim_inline float4 VEC_CALL operator / (float4 lhs, float4 rhs)
{
    return f4_div(lhs, rhs);
}
pim_inline float4 VEC_CALL operator / (float lhs, float4 rhs)
{
    return f4_divsv(lhs, rhs);
}
pim_inline float4 VEC_CALL operator / (float4 lhs, float rhs)
{
    return f4_divvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator == (float4 lhs, float4 rhs)
{
    return f4_eq(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator == (float lhs, float4 rhs)
{
    return f4_eqvs(rhs, lhs);
}
pim_inline bool4 VEC_CALL operator == (float4 lhs, float rhs)
{
    return f4_eqvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator != (float4 lhs, float4 rhs)
{
    return f4_neq(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator != (float lhs, float4 rhs)
{
    return f4_neqvs(rhs, lhs);
}
pim_inline bool4 VEC_CALL operator != (float4 lhs, float rhs)
{
    return f4_neqvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator < (float4 lhs, float4 rhs)
{
    return f4_lt(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator < (float lhs, float4 rhs)
{
    return f4_ltsv(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator < (float4 lhs, float rhs)
{
    return f4_ltvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator <= (float4 lhs, float4 rhs)
{
    return f4_lteq(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator <= (float lhs, float4 rhs)
{
    return f4_lteqsv(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator <= (float4 lhs, float rhs)
{
    return f4_lteqvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator > (float4 lhs, float4 rhs)
{
    return f4_gt(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator > (float lhs, float4 rhs)
{
    return f4_gtsv(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator > (float4 lhs, float rhs)
{
    return f4_gtvs(lhs, rhs);
}

pim_inline bool4 VEC_CALL operator >= (float4 lhs, float4 rhs)
{
    return f4_gteq(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator >= (float lhs, float4 rhs)
{
    return f4_gteqsv(lhs, rhs);
}
pim_inline bool4 VEC_CALL operator >= (float4 lhs, float rhs)
{
    return f4_gteqvs(lhs, rhs);
}

pim_inline float4 VEC_CALL min(float4 a, float4 b)
{
    return f4_min(a, b);
}
pim_inline float4 VEC_CALL min(float a, float4 b)
{
    return f4_minsv(a, b);
}
pim_inline float4 VEC_CALL min(float4 a, float b)
{
    return f4_minvs(a, b);
}

pim_inline float4 VEC_CALL max(float4 a, float4 b)
{
    return f4_max(a, b);
}
pim_inline float4 VEC_CALL max(float a, float4 b)
{
    return f4_maxsv(a, b);
}
pim_inline float4 VEC_CALL max(float4 a, float b)
{
    return f4_maxvs(a, b);
}

pim_inline float4 VEC_CALL select(float4 a, float4 b, bool4 t)
{
    return f4_select(a, b, t);
}
pim_inline float4 VEC_CALL select(float4 a, float4 b, bool t)
{
    return f4_selectvs(a, b, t);
}
pim_inline float4 VEC_CALL select(float a, float b, bool4 t)
{
    return f4_selectsv(a, b, t);
}

pim_inline float VEC_CALL hsum(float4 v)
{
    return f4_sum(v);
}
pim_inline float VEC_CALL hsum3(float4 v)
{
    return f4_sum3(v);
}

pim_inline float VEC_CALL hmin(float4 v)
{
    return f4_hmin(v);
}
pim_inline float VEC_CALL hmin3(float4 v)
{
    return f4_hmin3(v);
}

pim_inline float VEC_CALL hmax(float4 v)
{
    return f4_hmax(v);
}
pim_inline float VEC_CALL hmax3(float4 v)
{
    return f4_hmax3(v);
}

pim_inline float4 VEC_CALL clamp(float4 x, float4 lo, float4 hi)
{
    return f4_clamp(x, lo, hi);
}
pim_inline float4 VEC_CALL clamp(float4 x, float lo, float hi)
{
    return f4_clampvs(x, lo, hi);
}
pim_inline float4 VEC_CALL clamp(float x, float4 lo, float4 hi)
{
    return f4_clampsv(x, lo, hi);
}

pim_inline float VEC_CALL dot(float4 a, float4 b)
{
    return f4_dot(a, b);
}
pim_inline float VEC_CALL dot3(float4 a, float4 b)
{
    return f4_dot3(a, b);
}
pim_inline float VEC_CALL dotsat(float4 a, float4 b)
{
    return f4_dotsat(a, b);
}

pim_inline float4 VEC_CALL cross3(float4 a, float4 b)
{
    return f4_cross3(a, b);
}

pim_inline float VEC_CALL length(float4 x)
{
    return f4_length(x);
}
pim_inline float VEC_CALL length3(float4 x)
{
    return f4_length3(x);
}

pim_inline float4 VEC_CALL normalize(float4 x)
{
    return f4_normalize(x);
}
pim_inline float4 VEC_CALL normalize3(float4 x)
{
    return f4_normalize3(x);
}

pim_inline float VEC_CALL distance(float4 a, float4 b)
{
    return f4_distance(a, b);
}
pim_inline float VEC_CALL distance3(float4 a, float4 b)
{
    return f4_distance3(a, b);
}

pim_inline float VEC_CALL lengthsq(float4 x)
{
    return f4_lengthsq(x);
}
pim_inline float VEC_CALL lengthsq3(float4 x)
{
    return f4_lengthsq3(x);
}

pim_inline float VEC_CALL distancesq(float4 a, float4 b)
{
    return f4_distancesq(a, b);
}
pim_inline float VEC_CALL distancesq3(float4 a, float4 b)
{
    return f4_distancesq3(a, b);
}

pim_inline float4 VEC_CALL lerp(float4 a, float4 b, float4 t)
{
    return f4_lerp(a, b, t);
}
pim_inline float4 VEC_CALL lerp(float4 a, float4 b, float t)
{
    return f4_lerpvs(a, b, t);
}
pim_inline float4 VEC_CALL lerp(float a, float b, float4 t)
{
    return f4_lerpsv(a, b, t);
}

pim_inline float4 VEC_CALL unlerp(float4 a, float4 b, float4 t)
{
    return f4_unlerp(a, b, t);
}
pim_inline float4 VEC_CALL unlerp(float4 a, float4 b, float t)
{
    return f4_unlerpvs(a, b, t);
}
pim_inline float4 VEC_CALL unlerp(float a, float b, float4 t)
{
    return f4_unlerpsv(a, b, t);
}

pim_inline float4 VEC_CALL remap(float4 x, float4 a, float4 b, float4 s, float4 t)
{
    return f4_remap(x, a, b, s, t);
}

pim_inline float4 VEC_CALL saturate(float4 x)
{
    return f4_saturate(x);
}

pim_inline float4 VEC_CALL step(float4 a, float4 b)
{
    return f4_step(a, b);
}

pim_inline float4 VEC_CALL unormstep(float4 t)
{
    return f4_unormstep(t);
}

pim_inline float4 VEC_CALL smoothstep(float4 a, float4 b, float4 x)
{
    return f4_smoothstep(a, b, x);
}
pim_inline float4 VEC_CALL smoothstep(float4 a, float4 b, float x)
{
    return f4_smoothstepvs(a, b, x);
}
pim_inline float4 VEC_CALL smoothstep(float a, float b, float4 x)
{
    return f4_smoothstepsv(a, b, x);
}

pim_inline float4 VEC_CALL reflect(float4 i, float4 n)
{
    return f4_reflect(i, n);
}
pim_inline float4 VEC_CALL reflect3(float4 i, float4 n)
{
    return f4_reflect3(i, n);
}

pim_inline float4 VEC_CALL refract(float4 i, float4 n, float eta)
{
    return f4_refract(i, n, eta);
}
pim_inline float4 VEC_CALL refract3(float4 i, float4 n, float eta)
{
    return f4_refract3(i, n, eta);
}

pim_inline float4 VEC_CALL sqrt(float4 v)
{
    return f4_sqrt(v);
}
pim_inline float4 VEC_CALL abs(float4 v)
{
    return f4_abs(v);
}

pim_inline float4 VEC_CALL pow(float4 v, float4 e)
{
    return f4_pow(v, e);
}
pim_inline float4 VEC_CALL pow(float4 v, float e)
{
    return f4_powvs(v, e);
}
pim_inline float4 VEC_CALL pow(float v, float4 e)
{
    return f4_powsv(v, e);
}

pim_inline float4 VEC_CALL exp(float4 v)
{
    return f4_exp(v);
}
pim_inline float4 VEC_CALL log(float4 v)
{
    return f4_log(v);
}
pim_inline float4 VEC_CALL sin(float4 v)
{
    return f4_sin(v);
}
pim_inline float4 VEC_CALL cos(float4 v)
{
    return f4_cos(v);
}
pim_inline float4 VEC_CALL tan(float4 v)
{
    return f4_tan(v);
}
pim_inline float4 VEC_CALL asin(float4 v)
{
    return f4_asin(v);
}
pim_inline float4 VEC_CALL acos(float4 v)
{
    return f4_acos(v);
}
pim_inline float4 VEC_CALL atan(float4 v)
{
    return f4_atan(v);
}
pim_inline float4 VEC_CALL floor(float4 v)
{
    return f4_floor(v);
}
pim_inline float4 VEC_CALL ceil(float4 v)
{
    return f4_ceil(v);
}
pim_inline float4 VEC_CALL trunc(float4 v)
{
    return f4_trunc(v);
}
pim_inline float4 VEC_CALL frac(float4 v)
{
    return f4_frac(v);
}

pim_inline float4 VEC_CALL fmod(float4 a, float4 b)
{
    return f4_fmod(a, b);
}
pim_inline float4 VEC_CALL fmod(float a, float4 b)
{
    return f4_fmodsv(a, b);
}
pim_inline float4 VEC_CALL fmod(float4 a, float b)
{
    return f4_fmodvs(a, b);
}

pim_inline float4 VEC_CALL radians(float4 v)
{
    return f4_rad(v);
}
pim_inline float4 VEC_CALL degrees(float4 v)
{
    return f4_deg(v);
}

pim_inline float4 VEC_CALL blend(float4 a, float4 b, float4 c, float4 wuvt)
{
    return f4_blend(a, b, c, wuvt);
}

pim_inline float4 VEC_CALL unorm(float4 s)
{
    return f4_unorm(s);
}
pim_inline float4 VEC_CALL snorm(float4 u)
{
    return f4_snorm(u);
}
