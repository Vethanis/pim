#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include "math/scalar.h"
#include "math/types.h"
#include "math/float4_funcs.h"

pim_inline u16 VEC_CALL f4_rgb5a1(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mulvs(v, 31.0f), 0.5f);
    u16 r = (u16)v.x;
    u16 g = (u16)v.y;
    u16 b = (u16)v.z;
    u16 c = (r << 11) | (g << 6) | (b << 1) | 1;
    return c;
}

pim_inline u32 VEC_CALL f4_rgba8(float4 v)
{
    v = f4_saturate(v);
    v = f4_addvs(f4_mulvs(v, 255.0f), 0.5f);
    u32 r = (u32)v.x;
    u32 g = (u32)v.y;
    u32 b = (u32)v.z;
    u32 a = (u32)v.w;
    u32 c = (a << 24) | (b << 16) | (g << 8) | r;
    return c;
}

pim_inline float4 VEC_CALL rgba8_f4(u32 c)
{
    const float s = 1.0f / 255.0f;
    u32 r = c & 0xff;
    u32 g = (c >> 8) & 0xff;
    u32 b = (c >> 16) & 0xff;
    u32 a = (c >> 24) & 0xff;
    return f4_v((r + 0.5f) * s, (g + 0.5f) * s, (b + 0.5f) * s, (a + 0.5f) * s);
}

// reference sRGB -> Linear conversion (no approximation)
pim_inline float VEC_CALL sRGBToLinear(float c)
{
    if (c <= 0.04045f)
    {
        return c / 12.92f;
    }
    return powf((c + 0.055f) / 1.055f, 2.4f);
}

// reference Linear -> sRGB conversion (no approximation)
pim_inline float VEC_CALL LinearTosRGB(float c)
{
    if (c <= 0.0031308f)
    {
        return c * 12.92f;
    }
    return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
}

// cubic fit sRGB -> Linear conversion
// max error = 0.001214
pim_inline float VEC_CALL f1_tolinear(float c)
{
    return 0.020883f * c + 0.656075f * (c*c) + 0.324285f * (c*c*c);
}

// cubic fit sRGB -> Linear conversion
// max error = 0.001214
pim_inline float4 VEC_CALL f4_tolinear(float4 c)
{
    return f4_add(f4_add(f4_mulvs(c, 0.020883f), f4_mulvs(f4_mul(c, c), 0.656075f)), f4_mulvs(f4_mul(c, f4_mul(c, c)), 0.324285f));
}

// cubic root fit Linear -> sRGB conversion
// max error = 0.003662
pim_inline float VEC_CALL f1_tosrgb(float c)
{
    float s1 = sqrtf(c);
    float s2 = sqrtf(s1);
    float s3 = sqrtf(s2);
    return 0.658444f * s1 + 0.643378f * s2 - 0.298148f * s3;
}

// cubic root fit Linear -> sRGB conversion
// max error = 0.003662
pim_inline float4 VEC_CALL f4_tosrgb(float4 c)
{
    float4 s1 = f4_sqrt(c);
    float4 s2 = f4_sqrt(s1);
    float4 s3 = f4_sqrt(s2);
    return f4_add(f4_add(f4_mulvs(s1, 0.658444f), f4_mulvs(s2, 0.643378f)), f4_mulvs(s3, -0.298148f));
}

pim_inline u32 VEC_CALL DirectionToColor(float4 dir)
{
    u32 c = f4_rgba8(f4_unorm(f4_normalize3(dir)));
    c |= 0xff << 24;
    return c;
}

pim_inline float4 VEC_CALL ColorToDirection_fast(u32 c)
{
    return f4_snorm(rgba8_f4(c));
}
pim_inline float4 VEC_CALL ColorToDirection(u32 c)
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

pim_inline u32 VEC_CALL LinearToColor(float4 lin)
{
    float4 sRGB = f4_tosrgb(lin);
    u32 color = f4_rgba8(sRGB);
    return color;
}

pim_inline float4 VEC_CALL ColorToLinear(u32 c)
{
    float4 sRGB = rgba8_f4(c);
    float4 linear = f4_tolinear(sRGB);
    return linear;
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

pim_inline float VEC_CALL tmap1_reinhard(float x)
{
    float y = x / (1.0f + x);
    return y;
}

pim_inline float4 VEC_CALL tmap4_reinhard(float4 x)
{
    float4 y = f4_div(x, f4_addvs(x, 1.0f));
    return y;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
pim_inline float VEC_CALL tmap1_aces(float x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    float y = (x * (a * x + b)) / (x * (c * x + d) + e);
    return y;
}

pim_inline float4 VEC_CALL tmap4_aces(float4 x)
{
    float4 y;
    y.x = tmap1_aces(x.x);
    y.y = tmap1_aces(x.y);
    y.z = tmap1_aces(x.z);
    y.w = 1.0f;
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
pim_inline float VEC_CALL tmap1_filmic(float x)
{
    x = f1_max(0.0f, x - 0.004f);
    float y = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
    return powf(y, 2.2f); // originally fit to gamma2.2
}

pim_inline float4 VEC_CALL tmap4_filmic(float4 x)
{
    float4 y;
    y.x = tmap1_filmic(x.x);
    y.y = tmap1_filmic(x.y);
    y.z = tmap1_filmic(x.z);
    y.w = 1.0f;
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
pim_inline float VEC_CALL tmap1_uchart2(float x)
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

pim_inline float4 VEC_CALL tmap4_uchart2(float4 x)
{
    float4 y;
    y.x = tmap1_uchart2(x.x);
    y.y = tmap1_uchart2(x.y);
    y.z = tmap1_uchart2(x.z);
    y.w = tmap1_uchart2(x.w);
    y = f4_divvs(y, y.w);
    return y;
}

// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// params: Shoulder Strength, Linear Strength, Linear Angle, Toe Strength
pim_inline float VEC_CALL tmap1_hable(float x, float4 params)
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

pim_inline float4 VEC_CALL tmap4_hable(float4 x, float4 params)
{
    float4 y;
    y.x = tmap1_hable(x.x, params);
    y.y = tmap1_hable(x.y, params);
    y.z = tmap1_hable(x.z, params);
    y.w = tmap1_hable(x.w, params);
    y = f4_divvs(y, y.w);
    return y;
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
