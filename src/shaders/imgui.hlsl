#include "TextureTable.hlsl"

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
    float4 texColor = SampleTable(kTextureIndex, input.uv);
    texColor.a = kDiscardAlpha != 0 ? 1.0 : texColor.a;
    PSOutput output;
    output.color = texColor * input.color;
    return output;
}
