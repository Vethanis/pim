#include "TextureTable.hlsl"
#include "Color.hlsl"

struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_Target0;
};

[[vk::push_constant]]
cbuffer Constants
{
    float2 kScale;
    float2 kTranslate;

    uint kTextureIndex;
    uint kDiscardAlpha;
    uint2 kPad;
};

PSInput VSMain(VSInput input)
{
    float2 pos = input.position * kScale + kTranslate;
    PSInput output;
    output.position = float4(pos.xy, 0.0, 1.0);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

PSOutput PSMain(PSInput input)
{
    float4 texColor = SampleTable2D(kTextureIndex, input.uv);
    texColor.a = kDiscardAlpha != 0 ? 1.0 : texColor.a;
    float4 color = texColor * input.color;

    if (cameraData.hdrEnabled != 0.0)
    {
        const float pqRange = cameraData.uiNits * (1.0 / 10000.0);
        color.rgb = Color_SceneToHDR(color.rgb);
        color.rgb = PQ_OOTF(color.rgb);
        color.rgb *= pqRange;
        color.rgb = PQ_InverseEOTF(color.rgb);
    }
    else
    {
        color.rgb = Color_SceneToSDR(color.rgb);
    }

    PSOutput output;
    output.color = saturate(color);
    return output;
}
