#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/types.h"
#include "math/float4_funcs.h"

// x: { r.x, g.x, b.x, wp.x } tristimulus coordinates rgb and whitepoint
// y: { r.y, g.y, b.y, wp.y } tristimulus coordinates rgb and whitepoint
// returns: 3x3 matrix to convert an RGB value in xy colorspace to a CIE XYZ coordinate
float3x3 VEC_CALL Color_RGB_XYZ(float4 x, float4 y);

// x: { r.x, g.x, b.x, wp.x } tristimulus coordinates rgb and whitepoint
// y: { r.y, g.y, b.y, wp.y } tristimulus coordinates rgb and whitepoint
// returns: 3x3 matrix to convert a CIE XYZ cordinate to an RGB value in xy colorspace
// This is the inverse of the RGB_XYZ matrix (you can use f3x3_inverse)
float3x3 VEC_CALL Color_XYZ_RGB(float4 x, float4 y);

void Color_DumpConversionMatrices(void);

pim_inline float4 VEC_CALL f4_Rec709_XYZ(float4 c)
{
    const float4 c0 = { 0.41239089f, 0.21263906f, 0.019330805f };
    const float4 c1 = { 0.35758442f, 0.71516883f, 0.11919476f };
    const float4 c2 = { 0.18048081f, 0.072192319f, 0.9505322f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_XYZ_Rec709(float4 c)
{
    const float4 c0 = { 3.2409692f, -0.96924347f, 0.055630092f };
    const float4 c1 = { -1.5373828f, 1.8759671f, -0.20397688f };
    const float4 c2 = { -0.49861068f, 0.041555069f, 1.0569714f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec2020_XYZ(float4 c)
{
    const float4 c0 = { 0.63695818f, 0.26270026f, 0.0f };
    const float4 c1 = { 0.14461692f, 0.67799813f, 0.028072689f };
    const float4 c2 = { 0.16888095f, 0.059301712f, 1.060985f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_XYZ_Rec2020(float4 c)
{
    const float4 c0 = { 1.7166507f, -0.66668427f, 0.017639853f };
    const float4 c1 = { -0.35567072f, 1.6164811f, -0.042770606f };
    const float4 c2 = { -0.25336623f, 0.015768535f, 0.94210321f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_AP0_XYZ(float4 c)
{
    const float4 c0 = { 0.95255238f, 0.34396645f, 0.0f };
    const float4 c1 = { 0.0f, 0.72816604f, 0.0f };
    const float4 c2 = { 9.3678616e-05f, -0.072132535f, 1.0088251f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_XYZ_AP0(float4 c)
{
    const float4 c0 = { 1.049811f, -0.49590304f, 0.0f };
    const float4 c1 = { -0.0f, 1.3733131f, -0.0f };
    const float4 c2 = { -9.7484539e-05f, 0.098240048f, 0.99125206f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_AP1_XYZ(float4 c)
{
    const float4 c0 = { 0.66245425f, 0.27222878f, -0.0055746892f };
    const float4 c1 = { 0.13400419f, 0.67408162f, 0.0040607289f };
    const float4 c2 = { 0.15618764f, 0.053689498f, 1.0103388f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_XYZ_AP1(float4 c)
{
    const float4 c0 = { 1.6410233f, -0.66366309f, 0.011721959f };
    const float4 c1 = { -0.32480329f, 1.6153321f, -0.0082844514f };
    const float4 c2 = { -0.23642468f, 0.016756363f, 0.98839515f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_AP0_AP1(float4 c)
{
    const float4 c0 = { 1.4514393161f, -0.0765537734f, 0.0083161484f };
    const float4 c1 = { -0.2365107469f, 1.1762296998f, -0.0060324498f };
    const float4 c2 = { -0.2149285693f, -0.0996759264f, 0.9977163014f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_AP1_AP0(float4 c)
{
    const float4 c0 = { 0.6954522414f, 0.0447945634f, -0.0055258826f };
    const float4 c1 = { 0.1406786965f, 0.8596711185f, 0.0040252103f };
    const float4 c2 = { 0.1638690622f, 0.0955343182f, 1.0015006723f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec709_Rec2020(float4 c)
{
    const float4 c0 = { 0.62740386f, 0.06909731f, 0.016391428f };
    const float4 c1 = { 0.32928303f, 0.91954052f, 0.088013299f };
    const float4 c2 = { 0.043313056f, 0.011362299f, 0.89559537f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_Rec2020_Rec709(float4 c)
{
    const float4 c0 = { 1.660491f, -0.12455052f, -0.018150739f };
    const float4 c1 = { -0.587641f, 1.1328998f, -0.10057887f };
    const float4 c2 = { -0.072849929f, -0.0083494037f, 1.1187295f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec709_AP0(float4 c)
{
    const float4 c0 = { 0.43293062f, 0.089413166f, 0.019161701f };
    const float4 c1 = { 0.37538442f, 0.81653321f, 0.11815205f };
    const float4 c2 = { 0.18937808f, 0.10302201f, 0.94221699f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_AP0_Rec709(float4 c)
{
    const float4 c0 = { 2.5583849f, -0.27798539f, -0.017170634f };
    const float4 c1 = { -1.11947f, 1.3660156f, -0.14852904f };
    const float4 c2 = { -0.391812f, -0.09348727f, 1.081018f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec709_AP1(float4 c)
{
    const float4 c0 = { 0.60310686f, 0.07011801f, 0.022178905f };
    const float4 c1 = { 0.32633454f, 0.91991681f, 0.11607833f };
    const float4 c2 = { 0.047995642f, 0.012763575f, 0.94101894f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_AP1_Rec709(float4 c)
{
    const float4 c0 = { 1.7312536f, -0.13161892f, -0.024568275f };
    const float4 c1 = { -0.60404283f, 1.1348411f, -0.12575033f };
    const float4 c2 = { -0.080107749f, -0.0086794198f, 1.0656365f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec2020_AP0(float4 c)
{
    const float4 c0 = { 0.66868573f, 0.044900179f, 0.0f };
    const float4 c1 = { 0.15181769f, 0.8621456f, 0.02782711f };
    const float4 c2 = { 0.17718965f, 0.10192245f, 1.0517036f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_AP0_Rec2020(float4 c)
{
    const float4 c0 = { 1.512861f, -0.079036415f, 0.0020912308f };
    const float4 c1 = { -0.25898734f, 1.1770666f, -0.031144103f };
    const float4 c2 = { -0.22978596f, -0.10075563f, 0.95350415f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline float4 VEC_CALL f4_Rec2020_AP1(float4 c)
{
    const float4 c0 = { 0.95993727f, 0.0016225278f, 0.0052900701f };
    const float4 c1 = { 0.01046662f, 0.9996857f, 0.023825262f };
    const float4 c2 = { 0.0070331395f, 0.0014901534f, 1.0501608f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}
pim_inline float4 VEC_CALL f4_AP1_Rec2020(float4 c)
{
    const float4 c0 = { 1.0417912f, -0.0016830862f, -0.0052097263f };
    const float4 c1 = { -0.010741562f, 1.0003656f, -0.022641439f };
    const float4 c2 = { -0.0069618821f, -0.0014082193f, 0.95230216f };
    return f4_add(f4_add(f4_mulvs(c0, c.x), f4_mulvs(c1, c.y)), f4_mulvs(c2, c.z));
}

pim_inline R5G5B5A1_t VEC_CALL f4_rgb5a1(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mul(v, f4_s(31.0f)), 0.5f);
    R5G5B5A1_t c;
    c.r = (u16)v.x;
    c.g = (u16)v.y;
    c.b = (u16)v.z;
    c.a = 1;
    return c;
}
pim_inline float4 VEC_CALL rgb5a1_f4(R5G5B5A1_t c)
{
    const float s = 1.0f / 31.0f;
    return f4_v(c.r * s, c.g * s, c.b * s, 1.0f);
}

pim_inline R8G8B8A8_t VEC_CALL f4_rgba8(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mulvs(v, 255.0f), 0.5f);
    R8G8B8A8_t c;
    c.r = (u32)v.x;
    c.g = (u32)v.y;
    c.b = (u32)v.z;
    c.a = (u32)v.w;
    return c;
}
pim_inline float4 VEC_CALL rgba8_f4(R8G8B8A8_t c)
{
    const float s = 1.0f / 255.0f;
    return f4_v(c.r * s, c.g * s, c.b * s, c.a * s);
}

pim_inline A2R10G10B10_t VEC_CALL f4_a2rgb10(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mul(v, f4_v(1023.0f, 1023.0f, 1023.0f, 3.0f)), 0.5f);
    A2R10G10B10_t c;
    c.r = (u32)v.x;
    c.g = (u32)v.y;
    c.b = (u32)v.z;
    c.a = (u32)v.w;
    return c;
}
pim_inline float4 VEC_CALL a2rgb10_f4(A2R10G10B10_t c)
{
    const float s = 1.0f / 1023.0f;
    const float t = 1.0f / 3.0f;
    return f4_v(c.r * s, c.g * s, c.b * s, c.a * t);
}

pim_inline R16G16B16A16_t VEC_CALL f4_rgba16(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mulvs(v, 65535.0f), 0.5f);
    R16G16B16A16_t c;
    c.r = (u32)v.x;
    c.g = (u32)v.y;
    c.b = (u32)v.z;
    c.a = (u32)v.w;
    return c;
}
pim_inline float4 VEC_CALL rgba16_f4(R16G16B16A16_t c)
{
    const float s = 1.0f / 65535.0f;
    return f4_v(c.r * s, c.g * s, c.b * s, c.a * s);
}

// reference sRGB EOTF
pim_inline float VEC_CALL f1_sRGB_EOTF(float V)
{
    if (V <= 0.04045f)
    {
        return V / 12.92f;
    }
    return powf((V + 0.055f) / 1.055f, 2.4f);
}

// reference sRGB Inverse EOTF
pim_inline float VEC_CALL f1_sRGB_InverseEOTF(float L)
{
    if (L <= 0.0031308f)
    {
        return L * 12.92f;
    }
    return 1.055f * powf(L, 1.0f / 2.4f) - 0.055f;
}

// cubic fit sRGB EOTF
// max error = 0.001214
pim_inline float VEC_CALL f1_sRGB_EOTF_Fit(float V)
{
    return 0.020883f * V + 0.656075f * (V*V) + 0.324285f * (V*V*V);
}

// cubic fit sRGB EOTF
// max error = 0.001214
pim_inline float4 VEC_CALL f4_sRGB_EOTF_Fit(float4 V)
{
    return f4_add(f4_add(f4_mulvs(V, 0.020883f), f4_mulvs(f4_mul(V, V), 0.656075f)), f4_mulvs(f4_mul(V, f4_mul(V, V)), 0.324285f));
}

// cubic root fit sRGB Inverse EOTF
// max error = 0.003662
pim_inline float VEC_CALL f1_sRGB_InverseEOTF_Fit(float L)
{
    float l1 = sqrtf(L);
    float l2 = sqrtf(l1);
    float l3 = sqrtf(l2);
    return 0.658444f * l1 + 0.643378f * l2 - 0.298148f * l3;
}

// cubic root fit sRGB Inverse EOTF
// max error = 0.003662
pim_inline float4 VEC_CALL f4_sRGB_InverseEOTF_Fit(float4 L)
{
    float4 l1 = f4_sqrt(L);
    float4 l2 = f4_sqrt(l1);
    float4 l3 = f4_sqrt(l2);
    return f4_add(f4_add(f4_mulvs(l1, 0.658444f), f4_mulvs(l2, 0.643378f)), f4_mulvs(l3, -0.298148f));
}

pim_inline R8G8B8A8_t VEC_CALL DirectionToColor(float4 dir)
{
    R8G8B8A8_t c = f4_rgba8(f4_unorm(f4_normalize3(dir)));
    c.a = 0xff;
    return c;
}

pim_inline float4 VEC_CALL ColorToDirection_fast(R8G8B8A8_t c)
{
    return f4_snorm(rgba8_f4(c));
}
pim_inline float4 VEC_CALL ColorToDirection(R8G8B8A8_t c)
{
    return f4_normalize3(ColorToDirection_fast(c));
}

pim_inline short2 VEC_CALL NormalTsToXy16(float4 n)
{
    ASSERT(n.z >= 0.0f); // must be in tangent space
    n = f4_mulvs(f4_normalize3(n), 32767.0f);
    short2 xy;
    xy.x = (i16)n.x;
    xy.y = (i16)n.y;
    return xy;
}

pim_inline float4 VEC_CALL Xy16ToNormalTs(short2 xy)
{
    float4 n;
    n.x = xy.x * (1.0f / (1 << 15));
    n.y = xy.y * (1.0f / (1 << 15));
    n.z = sqrtf(f1_max(0.0f, 1.0f - (n.x * n.x + n.y * n.y)));
    n.w = 0.0f;
    return n;
}

pim_inline R8G8B8A8_t VEC_CALL GammaEncode_rgba8(float4 lin)
{
    return f4_rgba8(f4_sRGB_InverseEOTF_Fit(lin));
}

pim_inline float4 VEC_CALL GammaDecode_rgba8(R8G8B8A8_t c)
{
    return f4_sRGB_EOTF_Fit(rgba8_f4(c));
}

// https://alienryderflex.com/hsp.html
pim_inline float VEC_CALL f4_hsplum(float4 x)
{
    const float4 coeff = { 0.299f, 0.587f, 0.114f, 0.0f };
    return sqrtf(f4_sum3(f4_mul(f4_mul(x, x), coeff)));
}

pim_inline float VEC_CALL f4_perlum(float4 x)
{
    return f4_dot3(x, f4_v(0.2126f, 0.7152f, 0.0722f, 0.0f));
}

// average luminosity
pim_inline float VEC_CALL f4_avglum(float4 x)
{
    return (x.x + x.y + x.z) / 3.0f;
}

// maximum luminosity
pim_inline float VEC_CALL f4_maxlum(float4 x)
{
    return f4_hmax3(x);
}

pim_inline float VEC_CALL f4_midlum(float4 x)
{
    return 0.5f * (f4_maxlum(x) + f4_avglum(x));
}

pim_inline float4 VEC_CALL f4_desaturate(float4 src, float amt)
{
    float lum = f4_perlum(src);
    return f4_lerpvs(src, f4_s(lum), amt);
}

pim_inline float4 VEC_CALL f4_setlum(float4 x, float oldLum, float newLum)
{
    return f4_mulvs(x, newLum / f1_max(kEpsilon, oldLum));
}

pim_inline float4 VEC_CALL f4_reinhard_lum(float4 x, float wp)
{
    float l0 = f1_max(f4_avglum(x), kEpsilon);
    float n = l0 * (1.0f + (l0 / (wp * wp)));
    float l1 = n / (1.0f + l0);
    return f4_setlum(x, l0, l1);
}

pim_inline float4 VEC_CALL f4_reinhard_rgb(float4 x, float wp)
{
    float4 n = f4_mul(x, f4_addvs(f4_divvs(x, wp * wp), 1.0f));
    return f4_div(n, f4_addvs(x, 1.0f));
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
pim_inline float VEC_CALL f1_aceskfit(float x)
{
    const float a = 2.43f; // <- modified to never clip
    const float b = 0.03f;
    const float d = 0.59f;
    const float e = 0.14f;
    float y = (x * (a * x + b)) / (x * (a * x + d) + e);
    return y;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
pim_inline float4 VEC_CALL f4_aceskfit(float4 x)
{
    float4 y;
    y.x = f1_aceskfit(x.x);
    y.y = f1_aceskfit(x.y);
    y.z = f1_aceskfit(x.z);
    y.w = 1.0f;
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
pim_inline float VEC_CALL f1_uncharted2(float x)
{
    const float a = 0.15f;
    const float b = 0.50f;
    const float c = 0.10f;
    const float d = 0.20f;
    const float e = 0.02f;
    const float f = 0.30f;
    float y = ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
pim_inline float4 VEC_CALL f4_uncharted2(float4 x, float wp)
{
    float4 y;
    y.x = f1_uncharted2(x.x);
    y.y = f1_uncharted2(x.y);
    y.z = f1_uncharted2(x.z);
    wp = f1_uncharted2(wp);
    y = f4_divvs(y, wp);
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// params: Shoulder Strength, Linear Strength, Linear Angle, Toe Strength
pim_inline float VEC_CALL f1_hable(float x, float4 params)
{
    const float A = params.x;
    const float B = params.y;
    const float C = params.z;
    const float D = params.w;
    const float E = 0.02f;
    const float F = 0.3f;
    float y = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// params: Shoulder Strength, Linear Strength, Linear Angle, Toe Strength
pim_inline float4 VEC_CALL f4_hable(float4 x, float4 params)
{
    float4 y;
    y.x = f1_hable(x.x, params);
    y.y = f1_hable(x.y, params);
    y.z = f1_hable(x.z, params);
    y.w = f1_hable(x.w, params);
    y = f4_divvs(y, y.w);
    return y;
}

// http://cdn2.gran-turismo.com/data/www/pdi_publications/PracticalHDRandWCGinGTS_20181222.pdf#page=184
// https://www.desmos.com/calculator/mbkwnuihbd
typedef struct pim_alignas(16) GTTonemapParams_s
{
    float P;    // [1, 100] (1.0)    whitepoint, shoulder asymptote
    float a;    // [0, 5]   (1.0)    linear section slope
    float m;    // [0, 1)   (0.22)   linear section height
    float l;    // [0, 1]   (0.4)    shoulder sharpness

    float c;    // [1, 3]   (1.33)   toe curvature
    float b;    // [0, 1]   (0.0)    bias, to avoid numerical instability near 0
    float _pad1;
    float _pad2;
} GTTonemapParams;

// http://cdn2.gran-turismo.com/data/www/pdi_publications/PracticalHDRandWCGinGTS_20181222.pdf#page=184
// https://www.desmos.com/calculator/mbkwnuihbd
pim_inline float VEC_CALL f1_GTTonemap(float x, GTTonemapParams gtp)
{
    const float P = gtp.P;
    const float a = gtp.a;
    const float m = gtp.m;
    const float l = gtp.l;
    const float c = gtp.c;
    const float b = gtp.b;
    const float l0 = ((P - m) * l) / a;
    const float S0 = m + l0;
    const float S1 = m + a * l0;
    const float C2 = (a * P) / (P - S1);

    float L = m + a * (x - m);
    float T = m * powf(x / m, c) + b;
    float S = P - (P - S1) * expf(-(C2 * (x - S0)) / P);
    float w0 = 1.0f - f1_smoothstep(0.0f, m, x);
    float w2 = (x < m + l0) ? 0.0f : 1.0f;
    float w1 = 1.0f - w0 - w2;
    return T * w0 + L * w1 + S * w2;
}

// http://cdn2.gran-turismo.com/data/www/pdi_publications/PracticalHDRandWCGinGTS_20181222.pdf#page=184
// https://www.desmos.com/calculator/mbkwnuihbd
pim_inline float4 VEC_CALL f4_GTTonemap(float4 x, GTTonemapParams gtp)
{
    x.x = f1_GTTonemap(x.x, gtp);
    x.y = f1_GTTonemap(x.y, gtp);
    x.z = f1_GTTonemap(x.z, gtp);
    return x;
}

// https://gpuopen.com/wp-content/uploads/2016/03/GdcVdrLottes.pdf
// p.x: contrast
// p.y: shoulder
// p.z: b anchor
// p.w: c anchor
// b: (-midIn^1 + hdrMax^a * midOut) / (((hdrMax^a)^d - (midIn^a)^d) * midOut)
// c: ((hdrMax^a)^d * midIn^a - hdrMax^a * (midIn^a)^d * midOut) / (((hdrMax^a)^d - (midIn^a)^d) * midOut)
pim_inline float VEC_CALL f1_lottes(float x, float4 p)
{
    float z = powf(x, p.x);
    float y = z / (powf(z, p.y) * p.z + p.w);
    return y;
}
pim_inline float4 VEC_CALL f4_lottes(float4 x, float4 p)
{
    float4 z = f4_powvs(x, p.x);
    float4 y = f4_div(z, f4_addvs(f4_mulvs(f4_powvs(z, p.y), p.z), p.w));
    return y;
}

// https://www.cl.cam.ac.uk/teaching/1718/AdvGraph/06_HDR_and_tone_mapping.pdf#page=29
// https://www.desmos.com/calculator/t5uzvc82ca
pim_inline float4 VEC_CALL f4_sigmoid(float4 x, float a, float b, float Lm)
{
    float4 t = f4_powvs(x, b);
    float u = powf(Lm / a, b);
    return f4_mulvs(f4_div(t, f4_addvs(t, u)), Lm);
}

// https://www.desmos.com/calculator/fzsmuukao3
// c: contrast [0.5, 4] (2)
// cp: contrast point [0.1, 1] (0.5)
// wp: whitepoint [1, +inf) (10)
pim_inline float4 VEC_CALL f4_HdrCurve(float4 x, float c, float cp, float wp)
{
    float4 a = f4_powvs(x, c);
    float4 b = f4_mulvs(f4_div(x, f4_addvs(x, wp)), wp);
    float t = f1_unormstep(f1_sat(f4_avglum(x) / cp));
    return f4_lerpvs(a, b, t);
}

// https://en.wikipedia.org/wiki/Transfer_functions_in_imaging
// OETF: Scene Luminance to Signal; (eg. Camera)
// EOTF: Signal to Display Luminance; (eg. Monitor)
// OOTF: Scene Luminance to Display Luminance; OETF(EOTF(x));

// Rec2100: https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2100-2-201807-I!!PDF-E.pdf

// E: Scene linear light
// Ep: Nonlinear color value
pim_inline float4 VEC_CALL f4_GRec709(float4 E)
{
    float4 a = f4_mulvs(E, 267.84f);
    float4 b = f4_subvs(f4_mulvs(f4_powvs(f4_mulvs(E, 59.5208f), 0.45f), 1.099f), 0.099f);
    float4 Ep = f4_select(a, b, f4_lteqvs(E, 0.0003024f));
    return Ep;
}

// Ep: Nonlinear color value
// Fd: Display linear light
pim_inline float4 VEC_CALL f4_GRec1886(float4 Ep)
{
    float4 Fd = f4_mulvs(f4_powvs(Ep, 2.4f), 100.0f);
    return Fd;
}

// E: Scene linear light
// Fd: Display linear light
pim_inline float4 VEC_CALL f4_PQ_OOTF(float4 E)
{
    float4 Fd = f4_GRec1886(f4_GRec709(E));
    return Fd;
}

// Ep: Nonlinear Signal in [0, 1]
// Fd: Display Luminance in [0, 10000] cd/m^2
pim_inline float4 VEC_CALL f4_PQ_EOTF(float4 Ep)
{
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    const float m1 = 0.15930175781;
    const float m2 = 78.84375;
    float4 t = f4_powvs(Ep, 1.0f / m2);
    float4 y = f4_div(f4_maxvs(f4_subvs(t, c1), 0.0f), f4_subsv(c2, f4_mulvs(t, c3)));
    float4 Y = f4_powvs(y, 1.0f / m1);
    float4 Fd = f4_mulvs(Y, 10000.0f);
    return Fd;
}

// Fd: Display Luminance in [0, 10000] cd/m^2
// Ep: Signal in [0, 1]
pim_inline float4 VEC_CALL f4_PQ_InverseEOTF(float4 Fd)
{
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    const float m1 = 0.15930175781;
    const float m2 = 78.84375;
    float4 Y = f4_mulvs(Fd, 1.0f / 10000.0f);
    float4 y = f4_powvs(Y, m1);
    float4 n = f4_addvs(f4_mulvs(y, c2), c1);
    float4 d = f4_addvs(f4_mulvs(y, c3), 1.0f);
    float4 Ep = f4_powvs(f4_div(n, d), m2);
    return Ep;
}

pim_inline float4 VEC_CALL f4_PQ_OETF(float4 E)
{
    return f4_PQ_InverseEOTF(f4_PQ_OOTF(E));
}

#define kEmissionScale 100.0f

pim_inline float VEC_CALL PackEmission(float4 emission)
{
    float e = f1_sat(f4_hmax3(emission) * (1.0f / kEmissionScale));
    return (e > kEpsilon) ? sqrtf(e) : 0.0f;
}

pim_inline float4 VEC_CALL UnpackEmission(float4 albedo, float e)
{
    return f4_mulvs(albedo, (e * e) * kEmissionScale);
}

pim_inline void VEC_CALL ConvertToMetallicRoughness(
    float4 diffuse,
    float4 specular,
    float glossiness,
    float4 *const pim_noalias albedoOut,
    float *const pim_noalias roughnessOut,
    float *const pim_noalias metallicOut)
{
    const float invSpecMax = 1.0f - f4_hmax3(specular);
    const float diffuseLum = f4_hsplum(diffuse);
    const float specLum = f4_hsplum(specular);

    float metallic = 0.0f;
    const float a = 0.04f;
    if (specLum > a)
    {
        float b = (diffuseLum * invSpecMax / (1.0f - a)) + (specLum - 2.0f * a);
        float c = a - specLum;
        float d = f1_max(kEpsilon, b * b - 4.0f * a * c);
        metallic = f1_sat((-b + sqrtf(d)) / (2.0f * a));
    }

    float4 diffAlbedo = f4_mulvs(diffuse, (invSpecMax / (1.0f - a)) / f1_max(kEpsilon, 1.0f - metallic));
    float4 specAlbedo = f4_divvs(f4_subvs(specular, a * (1.0f - metallic)), f1_max(kEpsilon, metallic));
    float4 albedo = f4_saturate(f4_lerpvs(diffAlbedo, specAlbedo, metallic * metallic));
    albedo.w = diffuse.w;

    float roughness = f1_sat(1.0f - glossiness);

    *albedoOut = albedo;
    *roughnessOut = roughness;
    *metallicOut = metallic;
}

PIM_C_END
