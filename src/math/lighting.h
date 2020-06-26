#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float4_funcs.h"
#include "math/float2_funcs.h"

PIM_C_BEGIN

// References:
// ----------------------------------------------------------------------------
// [Burley12]: https://blog.selfshadow.com/publications/s2012-shading-course/burley/s2012_pbs_disney_brdf_notes_v3.pdf
// [Karis13]: https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// [Karis14]: https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
// [Lagarde15]: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

// Legend:
// ----------------------------------------------------------------------------
// FRESNEL:
//   f0: fresnel reflectance at normal incidence
//   f90: fresnel reflectance at tangent incidence
// ROUGHNESS:
//   perceptually linear roughness: material.roughness
//   alpha, or a: pow(material.roughness, 2)
//   alpha_linear or a_lin: material.roughness
//   """
//     [Burley12]
//     For  roughness, we found that mapping alpha=roughness^2 results
//     in a more perceptually linear change in the roughness.
//     Without this remapping, very small and non-intuitive values were required
//     for matching shiny materials. Also, interpolating between a rough and
//     smooth material would always produce a rough result.
//   """

#define kMinDenom       (1.0f / (1<<10))
#define kMinLightDist   0.01f
#define kMinLightDistSq 0.001f
#define kMinAlpha       kMinDenom

typedef struct BrdfLut_s
{
    int2 size;
    float2* pim_noalias texels;
} BrdfLut;

BrdfLut BakeBRDF(int2 size, u32 numSamples);
void FreeBrdfLut(BrdfLut* lut);

pim_inline float VEC_CALL BrdfAlpha(float roughness)
{
    return f1_max(roughness * roughness, kMinAlpha);
}

// Specular reflectance at normal incidence (N==L)
// """
//   [Burley12, Page 16, Section 5.5]
//   The constant, F0, represents the specular reflectance at normal incidence
//   and is achromatic for dielectrics and chromatic (tinted) for metals.
//   The actual value depends on the index of refraction.
// """
pim_inline float4 VEC_CALL F_0(float4 albedo, float metallic)
{
    return f4_lerpvs(f4_s(0.04f), albedo, metallic);
}

// any F0 lower than 0.02 can be assumed as a form of pre-baked occlusion
pim_inline float VEC_CALL F_90(float4 F0)
{
    return f1_sat(50.0f * f4_dot3(F0, f4_s(0.33f)));
}

// Specular 'F' term
// represents the ratio of reflected to refracted light
// cosTheta may be HoL or HoV (they are equivalent)
// """
//   [Burley12, Page 16, Section 5.5]
//   The  Fresnel  function  can  be  seen  as  interpolating  (non-linearly)
//   between  the  incident  specular reflectance and unity at grazing angles.
//   Note that the response becomes achromatic at grazing incidenceas all light
//   is reflected.
// """
pim_inline float4 VEC_CALL F_Schlick(float4 f0, float4 f90, float cosTheta)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f4_lerpvs(f0, f90, t5);
}

pim_inline float VEC_CALL F_Schlick1(float f0, float f90, float cosTheta)
{
    float t = 1.0f - cosTheta;
    float t5 = t * t * t * t * t;
    return f1_lerp(f0, f90, t5);
}

// aka 'Specular Color'
pim_inline float4 VEC_CALL F_SchlickEx(
    float4 albedo,
    float metallic,
    float cosTheta)
{
    float4 f0 = F_0(albedo, metallic);
    float4 f90 = f4_s(F_90(f0));
    return F_Schlick(f0, f90, cosTheta);
}

pim_inline float4 VEC_CALL DiffuseColor(
    float4 F,
    float4 albedo,
    float metallic)
{
    return f4_mulvs(f4_mul(albedo, f4_inv(F)), 1.0f - metallic);
}

// Specular 'D' term
// represents the normal distribution function
// GGX Trowbridge-Reitz approximation
// [Karis13, Page 3, Figure 3]
pim_inline float VEC_CALL D_GTR(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float f = f1_lerp(1.0f, a2, NoH * NoH);
    return a2 / f1_max(kEpsilon, f * f * kPi);
}

// Specular 'D' term with SphereNormalization applied
// [Karis13, Page 16, Equation 14]
pim_inline float VEC_CALL D_GTR_Sphere(
    float NoH,
    float alpha,
    float alphaPrime)
{
    // original normalization factor: 1 / (pi * a^2)
    // sphere normalization factor: (a / a')^2
    float a2 = alpha * alpha;
    float ap2 = alphaPrime * alphaPrime;
    float f = f1_lerp(1.0f, a2, NoH * NoH);
    return (a2 * ap2) / f1_max(kEpsilon, f * f);
}

// Specular 'V' term (G term / denominator)
// Correlated Smith
// represents the self shadowing and masking of microfacets in rough materials
// [Lagarde15, Page 12, Listing 2]
pim_inline float VEC_CALL G_SmithGGX(float NoL, float NoV, float alpha)
{
    float a2 = alpha * alpha;
    float v = NoL * sqrtf(a2 + (NoV - NoV * a2) * NoV);
    float l = NoV * sqrtf(a2 + (NoL - NoL * a2) * NoL);
    return 0.5f / f1_max(kEpsilon, v + l);
}

pim_inline float VEC_CALL Fd_Lambert()
{
    return 1.0f / kPi;
}

// Diffuse term
// normalized Burley diffuse brdf
// [Lagarde15, Page 10, Listing 1]
pim_inline float VEC_CALL Fd_Burley(
    float NoL,
    float NoV,
    float HoV,
    float alpha)
{
    float energyBias = 0.5f * alpha;
    float energyFactor = f1_lerp(1.0f, 1.0f / 1.51f, alpha);
    float fd90 = energyBias + 2.0f * HoV * HoV * alpha;
    float lightScatter = F_Schlick1(1.0f, fd90, NoL);
    float viewScatter = F_Schlick1(1.0f, fd90, NoV);
    return (lightScatter * viewScatter * energyFactor) / kPi;
}

// multiply output by radiance of light
pim_inline float4 VEC_CALL DirectBRDF(
    float4 V,
    float4 L,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic)
{
    float4 H = f4_normalize3(f4_add(V, L));
    float NoV = f4_dotsat(N, V);
    float NoH = f4_dotsat(N, H);
    float NoL = f4_dotsat(N, L);
    float HoV = f4_dotsat(H, V);

    float alpha = BrdfAlpha(roughness);
    float4 F = F_SchlickEx(albedo, metallic, HoV);
    float G = G_SmithGGX(NoL, NoV, alpha);
    float D = D_GTR(NoH, alpha);
    float4 Fr = f4_mulvs(F, D * G);

    float4 Fd = f4_mulvs(
        DiffuseColor(F, albedo, metallic),
        Fd_Lambert());

    return f4_add(Fr, Fd);
}

// for area lights with varying diffuse and specular light directions
pim_inline float4x2 VEC_CALL DirectBRDFSplit(
    float4 V,
    float4 Ld,
    float4 Ls,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic)
{
    float alpha = BrdfAlpha(roughness);
    float NoV = f4_dotsat(N, V);

    float4 Fr;
    {
        float4 H = f4_normalize3(f4_add(V, Ls));
        float NoH = f4_dotsat(N, H);
        float NoL = f4_dotsat(N, Ls);
        float HoV = f4_dotsat(H, V);
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float D = D_GTR(NoH, alpha);
        Fr = f4_mulvs(F, D * G);
    }

    float4 Fd;
    {
        float4 H = f4_normalize3(f4_add(V, Ld));
        float NoL = f4_dotsat(N, Ld);
        float HoV = f4_dotsat(H, V);
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        Fd = f4_mulvs(
            DiffuseColor(F, albedo, metallic),
            Fd_Lambert());
    }

    float4x2 result = { Fd, Fr };
    return result;
}

// polynomial approximation for convolved specular DFG
// [Karis14]
pim_inline float4 VEC_CALL EnvBRDF(
    float4 F,
    float NoV,
    float alpha)
{
    const float4 c0 = { -1.0f, -0.0275f, -0.572f, 0.022f };
    const float4 c1 = { 1.0f, 0.0425f, 1.04f, -0.04f };
    float4 r = f4_add(f4_mulvs(c0, alpha), c1);
    float a004 = f1_min(r.x * r.x, exp2f(-9.28f * NoV)) * r.x + r.y;
    float a = -1.04f * a004 + r.z;
    float b = 1.04f * a004 + r.w;
    return f4_addvs(f4_mulvs(F, a), b);
}

pim_inline float4 VEC_CALL IndirectBRDF(
    float4 V,           // unit view vector pointing from surface to eye
    float4 N,           // unit normal vector pointing outward from surface
    float4 diffuseGI,   // low frequency scene irradiance
    float4 specularGI,  // high frequency scene irradiance
    float4 albedo,      // surface color
    float roughness,    // surface roughness
    float metallic,     // surface metalness
    float ao)           // 1 - ambient occlusion (affects gi only)
{
    float alpha = BrdfAlpha(roughness);
    float NoV = f4_dotsat(N, V);

    float4 F = F_SchlickEx(albedo, metallic, NoV);
    float4 Fr = EnvBRDF(F, NoV, alpha);
    Fr = f4_mul(Fr, specularGI);

    float4 Fd = f4_mul(DiffuseColor(F, albedo, metallic), diffuseGI);

    return f4_mulvs(f4_add(Fd, Fr), ao);
}

pim_inline float VEC_CALL SphereAttenRadius(float4 color, float radius)
{
    float lum = f4_dot3(color, f4_v(0.2126f, 0.7152f, 0.0722f, 0.0f));
    lum = f1_max(lum, kEpsilon);
    const float lumCutoff = 0.05f; // hopefully temporary hardcoding
    return radius * sqrtf(lum / lumCutoff);
}

pim_inline float VEC_CALL SmoothDistanceAtt(
    float distance, // distance squared between surface and light
    float attRadius) // attenuation radius
{
    float d2 = distance * distance;
    float f1 = d2 / f1_max(1.0f, attRadius * attRadius);
    float f2 = f1_saturate(1.0f - f1 * f1);
    return f2 * f2;
}

pim_inline float VEC_CALL DistanceAtt(float distance)
{
    return 1.0f / f1_max(kMinLightDistSq, distance * distance);
}

pim_inline float VEC_CALL AngleAtt(
    float4 L, // normalized light vector
    float4 Ldir, // spotlight direction
    float cosInner, // cosine of inner angle
    float cosOuter, // cosine of outer angle
    float angleScale) // width of inner to outer transition
{
    float scale = 1.0f / (cosInner - cosOuter);
    float offset = -cosOuter * angleScale;
    float cd = f4_dot3(Ldir, L);
    float att = f1_saturate(cd * scale + offset);
    return att * att;
}

pim_inline float4 VEC_CALL EvalPointLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 lightPos,
    float4 lightColor)
{
    float4 L0 = f4_sub(lightPos, P);
    float distance = f4_length3(L0);
    distance = f1_max(kMinLightDist, distance);
    float4 L = f4_divvs(L0, distance);

    float4 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
    float att = f4_dotsat(N, L) * DistanceAtt(distance);
    att *= SmoothDistanceAtt(distance, SphereAttenRadius(lightColor, kMinLightDist));
    return f4_mul(brdf, f4_mulvs(lightColor, att));
}

pim_inline float4 VEC_CALL EvalSunLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 sunDir) // sun direction
{
    // treat sun as a disk light
    // Earth's sun has an angular diameter between 0.526 and 0.545 degrees.
    const float kSunAngRadius = (0.536f / 2.0f) * kRadiansPerDegree;
    // Earth's sun has an illumiance between 105k and 114k lux.
    const float kSunLux = 110000.0f;
    // disk radius
    const float r = 0.00467746533f; // sinf(kSunAngRadius);
    // distance to disk
    const float d = 0.99998906059f; // cosf(kSunAngRadius);

    // D: diffuse lighting direction
    float4 D = sunDir;
    float4 R = f4_reflect3(f4_neg(V), N);
    float DoR = f4_dot3(D, R);

    // L: specular lighting direction
    float4 L = R;
    if (DoR < d)
    {
        float4 S = f4_sub(R, f4_mulvs(D, DoR));
        S = f4_normalize3(S);
        S = f4_mulvs(S, r);
        L = f4_add(f4_mulvs(D, d), S);
        L = f4_normalize3(L);
    }

    float illuminance = kSunLux * f4_dotsat(N, D);
    float4x2 FdFr = DirectBRDFSplit(V, D, L, N, albedo, roughness, metallic);
    float4 brdf = f4_add(FdFr.c0, FdFr.c1);

    return f4_mulvs(brdf, illuminance);
}

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

pim_inline float VEC_CALL SphereLumensToNits(float lumens, float radius)
{
    return lumens / (SphereArea(radius) * kPi);
}

pim_inline float VEC_CALL DiskLumensToNits(float lumens, float radius)
{
    return lumens / (DiskArea(radius) * kPi);
}

pim_inline float VEC_CALL TubeLumensToNits(
    float lumens,
    float radius,
    float width)
{
    return lumens / (TubeArea(radius, width) * kPi);
}

pim_inline float VEC_CALL RectLumensToNits(
    float lumens,
    float width,
    float height)
{
    return lumens / (RectArea(width, height) * kPi);
}

pim_inline float VEC_CALL SphereSinSigmaSq(float radius, float distance)
{
    radius = f1_max(kMinLightDist, radius);
    distance = f1_max(radius, distance);

    float r2 = radius * radius;
    float d2 = distance * distance;
    float sinSigmaSq = r2 / d2;
    sinSigmaSq = f1_min(sinSigmaSq, 1.0f - kMinDenom);
    return sinSigmaSq;
}

pim_inline float VEC_CALL DiskSinSigmaSq(float radius, float distance)
{
    radius = f1_max(kMinLightDist, radius);
    distance = f1_max(radius, distance);

    float r2 = radius * radius;
    float d2 = distance * distance;
    float sinSigmaSq = r2 / (r2 + d2);
    sinSigmaSq = f1_min(sinSigmaSq, 1.0f - kMinDenom);
    return sinSigmaSq;
}

// [Lagarde15, Page 47, Listing 10]
pim_inline float VEC_CALL SphereDiskIlluminance(
    float cosTheta,
    float sinSigmaSqr)
{
    cosTheta = f1_clamp(cosTheta, -0.9999f, 0.9999f);

    float c2 = cosTheta * cosTheta;

    float illuminance = 0.0f;
    if (c2 > sinSigmaSqr)
    {
        illuminance = kPi * sinSigmaSqr * f1_sat(cosTheta);
    }
    else
    {
        float sinTheta = sqrtf(1.0f - c2);
        float x = sqrtf(1.0f / sinSigmaSqr - 1.0f);
        float y = -x * (cosTheta / sinTheta);
        float sinThetaSqrtY = sinTheta * sqrtf(1.0f - y * y);

        float t1 = (cosTheta * acosf(y) - x * sinThetaSqrtY) * sinSigmaSqr;
        float t2 = atanf(sinThetaSqrtY / x);
        illuminance = t1 + t2;
    }

    return f1_max(illuminance, 0.0f);
}

// [Karis13, Page 14, Equation 10]
pim_inline float VEC_CALL AlphaPrime(
    float alpha,
    float radius,
    float distance)
{
    return f1_saturate(alpha + (radius / (2.0f * distance)));
}

pim_inline float4 VEC_CALL SpecularDominantDir(
    float4 N,
    float4 R,
    float alpha)
{
    return f4_normalize3(f4_lerpvs(R, N, alpha));
}

// [Karis13, page 15, equation 11]
pim_inline float4 VEC_CALL SphereRepresentativePoint(
    float4 R,
    float4 N,
    float4 L0, // unnormalized vector from surface to center of sphere
    float radius,
    float alpha)
{
    //R = SpecularDominantDir(N, R, alpha);
    float4 c2r = f4_sub(f4_mulvs(R, f4_dot3(L0, R)), L0);
    float t = f1_sat(radius / f4_length3(c2r));
    float4 cp = f4_add(L0, f4_mulvs(c2r, t));
    return cp;
}

pim_inline float4 VEC_CALL EvalSphereLight(
    float4 V,
    float4 P,
    float4 N,
    float4 albedo,
    float roughness,
    float metallic,
    float4 lightPos,
    float4 lightColor,
    float radius)
{
    float4 L0 = f4_sub(lightPos, P);
    float distance = f4_length3(L0);

    radius = f1_max(kMinLightDist, radius);
    distance = f1_max(distance, radius);

    float alpha = BrdfAlpha(roughness);
    float NoV = f4_dotsat(N, V);
    float attSmooth = SmoothDistanceAtt(distance, SphereAttenRadius(lightColor, radius));

    float4 Fr = f4_0;
    #if (1)
    {
        float4 R = f4_normalize3(f4_reflect3(f4_neg(V), N));
        float4 L = SphereRepresentativePoint(R, N, L0, radius, alpha);
        float specDist = f4_length3(L);
        L = f4_divvs(L, specDist);

        float alphaPrime = AlphaPrime(alpha, radius, distance);

        float4 H = f4_normalize3(f4_add(V, L));
        float NoH = f4_dotsat(N, H);
        float HoV = f4_dotsat(H, V);
        float NoL = f4_dotsat(N, L);
        float D = D_GTR_Sphere(NoH, alpha, alphaPrime);
        float G = G_SmithGGX(NoL, NoV, alpha);
        float4 F = F_SchlickEx(albedo, metallic, HoV);
        Fr = f4_mulvs(F, D * G);

        float I = NoL * DistanceAtt(distance) * attSmooth;
        Fr = f4_mulvs(Fr, I);
    }
    #endif

    float4 Fd = f4_0;
    #if (1)
    {
        float4 L = f4_divvs(L0, distance);
        float4 H = f4_normalize3(f4_add(V, L));
        float HoV = f4_dotsat(H, V);
        float NoL = f4_dot3(N, L);
        float4 F = F_SchlickEx(albedo, metallic, HoV);

        Fd = f4_mulvs(
            DiffuseColor(F, albedo, metallic),
            Fd_Lambert());

        float sinSigmaSqr = SphereSinSigmaSq(radius, distance);
        float I = SphereDiskIlluminance(NoL, sinSigmaSqr) * attSmooth;
        Fd = f4_mulvs(Fd, I);
    }
    #endif

    float4 light = f4_mul(f4_add(Fd, Fr), lightColor);

    return light;
}

// [Lagarde15, Page 85, Listing 28]
pim_inline float VEC_CALL CompEV100(
    float aperture,
    float shutterTime,
    float ISO)
{
    float a = aperture * aperture;
    float b = 1.0f / shutterTime;
    float c = 100.0f / ISO;
    return log2f(a * b * c);
}

pim_inline float VEC_CALL CompEV100FromAvgLum(float avgLum)
{
    const float c = 100.0f / 12.5;
    return log2f(avgLum * c);
}

pim_inline float VEC_CALL ConvEV100ToExposure(float EV100)
{
    float maxLum = 1.2f * exp2f(EV100);
    return 1.0f / maxLum;
}

pim_inline float4 VEC_CALL CompBloomLuminance(
    float4 bloomColor,
    float bloomEC,
    float currentEV)
{
    float bloomEV = currentEV + bloomEC;
    float t = exp2f(bloomEV - 3.0f);
    return f4_mulvs(bloomColor, t);
}

pim_inline float4 VEC_CALL ExposeColor(float4 color, float avgLum)
{
    float ev100 = CompEV100FromAvgLum(avgLum);
    float exposure = ConvEV100ToExposure(ev100);
    return f4_mulvs(color, exposure);
}

PIM_C_END
