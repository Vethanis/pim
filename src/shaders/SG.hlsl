#ifndef SG_HLSL
#define SG_HLSL

#include "Macro.hlsl"

float SG_BasisEval(float4 axis, float3 dir)
{
    float sharpness = axis.w;
    float cosTheta = dot(dir, axis.xyz);
    return exp(sharpness * (cosTheta - 1.0f));
}

float3 SG_Eval(float4 axis, float3 amplitude, float3 dir)
{
    return amplitude * SG_BasisEval(axis, dir);
}

float SG_BasisIntegral(float sharpness)
{
    float e = 1.0f - exp(sharpness * -2.0f);
    return kTau * (e / sharpness);
}

float3 SG_Integral(float4 axis, float3 amplitude)
{
    return amplitude * SG_BasisIntegral(axis.w);
}

float3 SG_Irradiance(float4 axis, float3 amplitude, float3 normal)
{
    float muDotN = dot(axis.xyz, normal);
    float lambda = axis.w;

    const float c0 = 0.36f;
    const float c1 = 1.0f / (4.0f * 0.36f);

    float eml = exp(-lambda);
    float eml2 = eml * eml;
    float rl = 1.0f / lambda;

    float scale = 1.0f + 2.0f * eml2 - rl;
    float bias = (eml - eml2) * rl - eml2;

    float x = sqrt(1.0f - scale);
    float x0 = c0 * muDotN;
    float x1 = c1 * x;

    float n = x0 + x1;

    float y = (abs(x0) <= x1) ? (n * n) / x : saturate(muDotN);

    float normalizedIrradiance = scale * y + bias;

    return SG_Integral(axis, amplitude) * normalizedIrradiance;
}

#endif // SG_HLSL
