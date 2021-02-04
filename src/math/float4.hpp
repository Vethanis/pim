#pragma once

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
