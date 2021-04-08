#include "Lighting.hlsl"
#include "Color.hlsl"
#include "Sampling.hlsl"
#include "TextureTable.hlsl"
#include "GI.hlsl"
#include "Exposure.hlsl"

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
    float3 positionWS : TEXCOORD0;
    float4 uv01 : TEXCOORD1;
    float3x3 TBN : TEXCOORD2;
    nointerpolation uint4 texIndices : TEXCOORD3;
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
    output.positionWS = positionWS.xyz;
    output.TBN = NormalToTBN(input.normalWS.xyz);
    output.uv01 = input.uv01;
    output.texIndices = input.texIndices;
    return output;
}

PSOutput PSMain(PSInput input)
{
    uint ai = input.texIndices.x;
    uint ri = input.texIndices.y;
    uint ni = input.texIndices.z;
    uint li = input.texIndices.w;
    float2 uv0 = input.uv01.xy;

    float3 albedo = SampleTable2D(ai, uv0).xyz;
    albedo = max(kEpsilon, albedo);
    float4 rome = SampleTable2D(ri, uv0);
    float3 normalTS = Xy16ToNormalTs(SampleTable2D(ni, uv0).xy);

    float3 N = TbnToWorld(input.TBN, normalTS);
    N = normalize(N);

    float3 P = input.positionWS;
    float3 V = normalize(cameraData.eye.xyz - P);

    float roughness = rome.x;
    float occlusion = rome.y;
    float metallic = rome.z;
    float emission = rome.w;
    float3 light = 0.0001 * albedo;
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
        float3 R = reflect(-V, N);
        GISample gi = SampleLightmap(li, uv1, input.TBN, N, R);
        float3 indirect = IndirectBRDF(V, N, gi.diffuse, gi.specular, albedo, roughness, metallic, occlusion);
        light += indirect;
    }

    PSOutput output;
    output.luminance = PerceptualLuminance(light);
    light *= GetExposure();
    output.color = float4(saturate(TonemapUncharted2(light)), 1.0);
    return output;
}
