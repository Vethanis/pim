#include "Lighting.hlsl"
#include "Color.hlsl"
#include "Sampling.hlsl"
#include "TextureTable.hlsl"
#include "GI.hlsl"
#include "Exposure.hlsl"

[[vk::push_constant]]
cbuffer push_constants
{
    float4x4 kLocalToWorld;
    float4 kIMc0;
    float4 kIMc1;
    float4 kIMc2;
    uint4 kTexInds;
};

struct VSInput
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
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
    float4 positionOS = float4(input.positionOS.xyz, 1.0);
    float4 positionWS = mul(kLocalToWorld, positionOS);
    float4 positionCS = mul(cameraData.worldToClip, positionWS);
    float3 normalWS = mul(float3x3(kIMc0.xyz, kIMc1.xyz, kIMc2.xyz), input.normalOS.xyz);

    PSInput output;
    output.positionCS = positionCS;
    output.positionWS = positionWS.xyz;
    output.TBN = NormalToTBN(normalWS.xyz);
    output.uv01 = input.uv01;
    output.texIndices = kTexInds;
    output.texIndices.w = input.texIndices.w;
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
    float3 sceneLum = 0.0001 * albedo;
    {
        float3 emissive = UnpackEmission(albedo, emission);
        sceneLum += emissive;
    }
    {
        //float3 L = cameraData[0].lightDir.xyz;
        //float3 lightColor = cameraData[0].lightColor.xyz;
        //float3 brdf = DirectBRDF(V, L, N, albedo, roughness, metallic);
        //float3 direct = brdf * lightColor * dotsat(N, L);
        //sceneLum += direct;
    }
    {
        float2 uv1 = input.uv01.zw;
        float3 R = reflect(-V, N);
        GISample gi = SampleLightmap(li, uv1, input.TBN, N, R);
        float3 indirect = IndirectBRDF(V, N, gi.diffuse, gi.specular, albedo, roughness, metallic, occlusion);
        sceneLum += indirect;
    }

    PSOutput output;
    output.luminance = AverageLuminance(sceneLum);
    sceneLum *= GetExposure();

    const float wp = cameraData.whitepoint;
    //const float nits = cameraData.displayNits;
    if (cameraData.hdrEnabled != 0.0)
    {
        float3 displayLum = PQ_OOTF(sceneLum);
        // This is a bit ridiculous, but it works.
        // multiple-exposure by projecting an hdr band
        // into the sdr range for an sdr tonemapper,
        // then projecting the result back to hdr range.
        float3 a = TonemapUncharted2(displayLum, wp);
        float3 b = TonemapUncharted2(displayLum * 0.1, wp) * 10.0;
        float3 c = TonemapUncharted2(displayLum * 0.01, wp) * 100.0;
        float3 d = TonemapUncharted2(displayLum * 0.001, wp) * 1000.0;
        float3 e = TonemapUncharted2(displayLum * 0.0001, wp) * 10000.0;
        displayLum = 0.2 * a + 0.2 * b + 0.2 * c + 0.2 * d + 0.2 * e;
        float3 signal = PQ_InverseEOTF(displayLum);
        output.color.rgb = signal;
    }
    else
    {
        output.color.rgb = TonemapUncharted2(sceneLum, wp);
    }

    output.color = float4(saturate(output.color.rgb), 1.0);
    return output;
}
