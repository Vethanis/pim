#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"
#include "math/color.h"

PIM_C_BEGIN

pim_inline float4 VEC_CALL f4_darken(float4 src, float4 blend)
{
    return f4_min(src, blend);
}

pim_inline float4 VEC_CALL f4_multiply(float4 src, float4 blend)
{
    return f4_mul(src, blend);
}

pim_inline float4 VEC_CALL f4_colorburn(float4 src, float4 blend)
{
    // 1 - (1 - src) / blend
    return f4_inv(f4_div(f4_inv(src), blend));
}

pim_inline float4 VEC_CALL f4_linearburn(float4 src, float4 blend)
{
    return f4_sub(f4_add(src, blend), f4_1);
}

pim_inline float4 VEC_CALL f4_lighten(float4 src, float4 blend)
{
    return f4_max(src, blend);
}

pim_inline float4 VEC_CALL f4_screen(float4 src, float4 blend)
{
    return f4_inv(f4_mul(f4_inv(src), f4_inv(blend)));
}

pim_inline float4 VEC_CALL f4_colordodge(float4 src, float4 blend)
{
    return f4_div(src, f4_max(f4_inv(blend), f4_s(f16_eps)));
}

pim_inline float4 VEC_CALL f4_lineardodge(float4 src, float4 blend)
{
    return f4_add(src, blend);
}

pim_inline float4 VEC_CALL f4_overlay(float4 src, float4 blend)
{
    // a = 2*src*blend
    float4 a = f4_mulvs(f4_mul(src, blend), 2.0f);
    // b = 1-(1-2*(src-0.5))*(1-blend)
    float4 b0 = f4_inv(f4_mulvs(f4_subvs(src, 0.5f), 2.0f));
    float4 b1 = f4_inv(blend);
    float4 b = f4_inv(f4_mul(b0, b1));
    // src <= 0.5 ? a : b
    return f4_select(b, a, f4_lteqvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_softlight(float4 src, float4 blend)
{
    float4 invSrc = f4_inv(src);
    // a = 1-(1-src)*(1-2*blend*src)
    float4 a1 = f4_inv(f4_mulvs(f4_mul(blend, src), 2.0f));
    float4 a = f4_inv(f4_mul(invSrc, a1));
    // b = src*(1-(1-src)*(1-2*blend))
    float4 b1 = f4_inv(f4_mulvs(blend, 2.0f));
    float4 b2 = f4_inv(f4_mul(invSrc, b1));
    float4 b = f4_mul(src, b2);
    // src <= 0.5 ? a : b
    return f4_select(b, a, f4_lteqvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_hardlight(float4 src, float4 blend)
{
    // a = 1-(1-src)*(1-2*blend)
    float4 a0 = f4_inv(src);
    float4 a1 = f4_inv(f4_mulvs(blend, 2.0f));
    float4 a = f4_inv(f4_mul(a0, a1));
    // b = 2*blend*src
    float4 b = f4_mulvs(f4_mul(blend, src), 2.0f);
    // src <= 0.5 ? a : b
    return f4_select(b, a, f4_lteqvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_vividlight(float4 src, float4 blend)
{
    // a = 1-(1-src)/(2*(blend-0.5))
    float4 a0 = f4_inv(src);
    float4 a1 = f4_mulvs(f4_subvs(blend, 0.5f), 2.0f);
    float4 a = f4_inv(f4_div(a0, f4_maxvs(a1, f16_eps)));
    // b = src/(1-2*blend)
    float4 b0 = f4_inv(f4_mulvs(blend, 2.0f));
    float4 b = f4_div(src, f4_maxvs(b0, f16_eps));
    // src > 0.5 ? a : b
    return f4_select(b, a, f4_gtvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_linearlight(float4 src, float4 blend)
{
    // a = src+2*(blend-0.5)
    float4 a = f4_add(src, f4_mulvs(f4_subvs(blend, 0.5f), 2.0f));
    // b = src+2*blend-1
    float4 b = f4_subvs(f4_add(src, f4_mulvs(blend, 2.0f)), 1.0f);
    // src <= 0.5 ? a : b
    return f4_select(b, a, f4_lteqvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_pinlight(float4 src, float4 blend)
{
    // a = max(src, 2*(blend-0.5))
    float4 a0 = f4_mulvs(f4_subvs(blend, 0.5f), 2.0f);
    float4 a = f4_max(src, a0);
    // b = min(src, 2*blend)
    float4 b = f4_min(src, f4_mulvs(blend, 2.0f));
    // src <= 0.5 ? a : b
    return f4_select(b, a, f4_lteqvs(blend, 0.5f));
}

pim_inline float4 VEC_CALL f4_difference(float4 src, float4 blend)
{
    return f4_abs(f4_sub(src, blend));
}

pim_inline float4 VEC_CALL f4_exclusion(float4 src, float4 blend)
{
    // 0.5 - 2 * (src-0.5) * (blend-0.5)
    float4 a = f4_subvs(src, 0.5f);
    float4 b = f4_subvs(blend, 0.5f);
    float4 c = f4_mulvs(f4_mul(a, b), 2.0f);
    return f4_subsv(0.5f, c);
}

pim_inline float4 VEC_CALL DiffuseToAlbedo(float4 diffuse)
{
    float lum = f4_perlum(diffuse);
    float4 decav = f4_softlight(diffuse, f4_s(1.0f - lum));
    float4 desat = f4_desaturate(decav, 0.1f);
    return desat;
}

PIM_C_END
