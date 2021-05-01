#ifndef GI_HLSL
#define GI_HLSL

#include "common.hlsl"
#include "SG.hlsl"
#include "Sampling.hlsl"

#define kGiDirections 5

static const float4 kGiAxii[kGiDirections] =
{
    { 0.000000, 0.000000, 1.000000, 4.999773 },
    { 0.577350, 0.577350, 0.577350, 4.999773 },
    { -0.577350, 0.577350, 0.577350, 4.999773 },
    { 0.577350, -0.577350, 0.577350, 4.999773 },
    { -0.577350, -0.577350, 0.577350, 4.999773 },
};

struct GISample
{
    float3 diffuse;
    float3 specular;
};

GISample SampleLightmap(
    uint lmIndex,
    float2 uv,
    float3x3 TBN,
    float3 N,
    float3 R)
{
    float3 d = 0.0;
    float3 s = 0.0;
    for (uint i = 0; i < kGiDirections; ++i)
    {
        float4 axis = kGiAxii[i];
        axis.xyz = TbnToWorld(TBN, axis.xyz);
        float3 probe = SampleTable2DArray(lmIndex, uv, i).xyz;
        d += SG_Irradiance(axis, probe, N);
        s += SG_Eval(axis, probe, R);
    }
    GISample output;
    output.diffuse = d;
    output.specular = s;
    return output;
}

#endif // GI_HLSL
