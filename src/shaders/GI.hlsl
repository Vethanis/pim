#ifndef GI_HLSL
#define GI_HLSL

#include "Macro.hlsl"
#include "TextureTable.hlsl"
#include "SG.hlsl"
#include "Sampling.hlsl"

#define kGiDirections 5

static const float4 kGiAxii[kGiDirections] =
{
    { 1.000000f, 0.000000f, 0.000000f, 3.210700f },
    { 0.267616f, 0.823639f, 0.500000f, 3.210700f },
    { -0.783327f, 0.569121f, 0.250000f, 3.210700f },
    { -0.535114f, -0.388783f, 0.750000f, 3.210700f },
    { 0.306594f, -0.943597f, 0.125000f, 3.210700f },
};

struct GISample
{
    float3 diffuse;
    float3 Reval;
    float3 Rirr;
};

uint GetLightmapIndex(uint baseIndex, uint vertexIndex)
{
    return baseIndex + vertexIndex * kGiDirections;
}

GISample SampleLightmap(
    uint lmIndex,
    float2 uv1,
    float3x3 TBN,
    float3 N,
    float3 R)
{
    float3 d = 0.0;
    float3 Reval = 0.0;
    float3 Rirr = 0.0;
    for (uint i = 0; i < kGiDirections; ++i)
    {
        float4 axis = kGiAxii[i];
        axis.xyz = TbnToWorld(TBN, axis.xyz);
        float3 probe = SampleTable(lmIndex + i, uv1).xyz;
        d += SG_Irradiance(axis, probe, N);
        Reval += SG_Eval(axis, probe, R);
        Rirr += SG_Irradiance(axis, probe, R);
    }
    GISample output;
    output.diffuse = d;
    output.Reval = Reval;
    output.Rirr = Rirr;
    return output;
}

#endif // GI_HLSL
