#include "common.hlsl"
#include "Color.hlsl"

struct VSInput
{
    float4 positionWS : POSITION;
    float4 normalWS : NORMAL;
    float4 uv01 : TEXCOORD0;
    uint4 texIndices : TEXCOORD1;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float2 uv0 : TEXCOORD0;
    nointerpolation uint albedoIndex : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_Target0;
    half luminance : SV_Target1;
};

PSInput VSMain(VSInput input)
{
    float4 positionWS = float4(input.positionWS.xyz, 1.0);
    float4 positionCS = mul(cameraData.worldToClip, positionWS);

    PSInput output;
    output.positionCS = positionCS;
    output.uv0 = input.uv01.xy;
    output.albedoIndex = input.texIndices.x;
    return output;
}

PSOutput PSMain(PSInput input)
{
    uint ai = input.albedoIndex;
    float2 uv0 = input.uv0;

    uv0.x = fmod(uv0.x, 0.5);
    float3 albedo = SampleTable2D(ai, uv0).xyz;
    if (dot(albedo, 0.33333) < 0.04)
    {
        uv0.x += 0.5;
        albedo = SampleTable2D(ai, uv0).xyz;
    }

    PSOutput output;
    output.luminance = 1.0;
    output.color = float4(ExposeScene(albedo), 1.0);
    return output;
}
