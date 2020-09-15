#include "Lighting.hlsl"
#include "Color.hlsl"
#include "Sampling.hlsl"
#include "SG.hlsl"

struct PerCamera
{
    float4x4 worldToClip;
    float4 eye;
    float4 giAxii[kGiDirections];
    uint lmBegin;
    float exposure;
};

struct VSInput
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float4 uv01 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float3 positionWS : TEXCOORD0;
    float4 uv01 : TEXCOORD1;
    float3x3 TBN : TEXCOORD2;
    nointerpolation uint lmIndex : TEXCOORD3;
};

struct PSOutput
{
    float4 color : SV_Target0;
    half luminance : SV_Target1;
};

[[vk::push_constant]]
cbuffer push_constants
{
    float4x4 localToWorld;
    float4 IMc0;
    float4 IMc1;
    float4 IMc2;
    float4 flatRome;
    uint kAlbedoIndex;
    uint kRomeIndex;
    uint kNormalIndex;
};

[[vk::binding(1)]]
cbuffer cameraData
{
    PerCamera cameraData;
};

[[vk::binding(2)]]
SamplerState samplers[];
[[vk::binding(2)]]
Texture2D textures[];

float4 SampleTexture(uint index, float2 uv)
{
    return textures[index].Sample(samplers[index], uv);
}

PSInput VSMain(VSInput input)
{
    float4 positionOS = float4(input.positionOS.xyz, 1.0);
    float4 positionWS = mul(localToWorld, positionOS);
    float4 positionCS = mul(cameraData.worldToClip, positionWS);
    float3x3 IM = float3x3(IMc0.xyz, IMc1.xyz, IMc2.xyz);
    float3 normalWS = mul(IM, input.normalOS.xyz);
    float3x3 TBN = NormalToTBN(normalWS);

    PSInput output;
    output.positionCS = positionCS;
    output.positionWS = positionWS.xyz;
    output.TBN = TBN;
    output.uv01 = input.uv01;
    output.lmIndex = (uint)input.normalOS.w;
    return output;
}

PSOutput PSMain(PSInput input)
{
    uint ai = kAlbedoIndex;
    uint ri = kRomeIndex;
    uint ni = kNormalIndex;
    float2 uv0 = input.uv01.xy;

    float3 albedo = SampleTexture(ai, uv0).xyz;
    albedo = max(kEpsilon, albedo);
    float4 rome = SampleTexture(ri, uv0) * flatRome;
    float3 normalTS = SampleTexture(ni, uv0).xyz;

    normalTS = normalize(normalTS * 2.0 - 1.0);
    float3 N = TbnToWorld(input.TBN, normalTS);
    N = normalize(N);

    float3 P = input.positionWS;
    float3 V = normalize(cameraData.eye.xyz - P);

    float roughness = rome.x;
    float occlusion = rome.y;
    float metallic = rome.z;
    float emission = rome.w;
    float3 light = 0.0;
    half luminance = 0.0;
    {
        float3 emissive = UnpackEmission(albedo, emission);
        light += emissive;
    }
    {
        //float3 L = cameraData[0].lightDir.xyz;
        //float3 lightColor = cameraData[0].lightColor.xyz;
        //float3 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
        //float3 direct = brdf * lightColor * dotsat(N, L);
        //light += direct;
    }
    {
        float2 uv1 = input.uv01.zw;
        uint lmIndex = cameraData.lmBegin + input.lmIndex * kGiDirections;
        float3 R = reflect(-V, N);
        float3 diffuseGI = 0.0;
        float3 specularGI = 0.0;
        for (uint i = 0; i < kGiDirections; ++i)
        {
            float4 ax = cameraData.giAxii[i];
            ax.w = max(ax.w, kEpsilon);
            ax.xyz = TbnToWorld(input.TBN, ax.xyz);
            float3 probe = SampleTexture(lmIndex + i, uv1).xyz;
            diffuseGI += SG_Irradiance(ax, probe, N);
            specularGI += SG_Eval(ax, probe, R);
        }
        luminance += PerceptualLuminance(diffuseGI);
        float3 indirect = IndirectBRDF(V, N, diffuseGI, specularGI, albedo, roughness, metallic, occlusion);
        light += indirect;
    }

    PSOutput output;
    output.luminance = luminance;
    light *= cameraData.exposure;
    output.color = float4(saturate(TonemapACES(light)), 1.0);
    return output;
}
