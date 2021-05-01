#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

#include "common.hlsl"

float BrdfAlpha(float roughness)
{
    return max(roughness * roughness, kMinAlpha);
}

float3 F_0(float3 albedo, float metallic)
{
    return lerp(0.04, albedo, metallic);
}

float F_90(float3 F0)
{
    return saturate(50.0f * dot(F0, 0.33));
}

float3 F_Schlick(float3 f0, float3 f90, float cosTheta)
{
    float t = 1.0 - cosTheta;
    float t5 = t * t * t * t * t;
    return lerp(f0, f90, t5);
}

float F_Schlick1(float f0, float f90, float cosTheta)
{
    float t = 1.0 - cosTheta;
    float t5 = t * t * t * t * t;
    return lerp(f0, f90, t5);
}

float3 F_SchlickEx(float3 albedo, float metallic, float cosTheta)
{
    float3 f0 = F_0(albedo, metallic);
    float f90 = F_90(f0);
    return F_Schlick(f0, f90, cosTheta);
}

float3 DiffuseColor(float3 albedo, float metallic)
{
    return albedo * (1.0 - metallic);
}

float D_GTR(float NoH, float alpha)
{
    float a2 = alpha * alpha;
    float f = lerp(1.0f, a2, NoH * NoH);
    return a2 / max(kEpsilon, f * f * kPi);
}

float G_SmithGGX(float NoL, float NoV, float alpha)
{
    float a2 = alpha * alpha;
    float v = NoL * sqrt(a2 + (NoV - NoV * a2) * NoV);
    float l = NoV * sqrt(a2 + (NoL - NoL * a2) * NoL);
    return 0.5f / max(kEpsilon, v + l);
}

float Fd_Lambert()
{
    return 1.0f / kPi;
}

float Fd_Burley(
    float NoL,
    float NoV,
    float HoV,
    float roughness)
{
    float fd90 = 0.5f + 2.0f * HoV * HoV * roughness;
    float lightScatter = F_Schlick1(1.0f, fd90, NoL);
    float viewScatter = F_Schlick1(1.0f, fd90, NoV);
    return (lightScatter * viewScatter) / kPi;
}

float3 DirectBRDF(
    float3 V,
    float3 L,
    float3 N,
    float3 albedo,
    float roughness,
    float metallic)
{
    float3 H = normalize(V + L);
    float NoV = dotsat(N, V);
    float NoH = dotsat(N, H);
    float NoL = dotsat(N, L);
    float HoV = dotsat(H, V);

    float alpha = BrdfAlpha(roughness);
    float3 F = F_SchlickEx(albedo, metallic, HoV);
    float G = G_SmithGGX(NoL, NoV, alpha);
    float D = D_GTR(NoH, alpha);
    float3 Fr = D * G * F;

    float3 Fd = DiffuseColor(albedo, metallic) * Fd_Burley(NoL, NoV, HoV, roughness);
    Fd *= 1.0 - F;

    return Fr + Fd;
}

float3 EnvBRDF(
    float3 F,
    float NoV,
    float alpha)
{
    const float4 c0 = { -1.0f, -0.0275f, -0.572f, 0.022f };
    const float4 c1 = { 1.0f, 0.0425f, 1.04f, -0.04f };
    float4 r = c0 * alpha + c1;
    float a004 = min(r.x * r.x, exp2(-9.28f * NoV)) * r.x + r.y;
    float a = -1.04f * a004 + r.z;
    float b = 1.04f * a004 + r.w;
    return F * a + b;
}

float3 IndirectBRDF(
    float3 V,           // unit view vector pointing from surface to eye
    float3 N,           // unit normal vector pointing outward from surface
    float3 diffuseGI,   // low frequency scene irradiance
    float3 specularGI,  // high frequency scene irradiance
    float3 albedo,      // surface color
    float roughness,    // perceptual roughness
    float metallic,
    float occlusion)
{
    float alpha = BrdfAlpha(roughness);
    float NoV = dotsat(N, V);

    float3 F = F_SchlickEx(albedo, metallic, NoV);
    float3 Fr = EnvBRDF(F, NoV, alpha);
    Fr = Fr * specularGI;

    float3 Fd = DiffuseColor(albedo, metallic) * diffuseGI;
    Fd *= 1.0 - F;

    return (Fr + Fd) * occlusion;
}

#endif // LIGHTING_HLSL
