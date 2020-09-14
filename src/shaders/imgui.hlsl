
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
    half luminance : SV_Target1;
};

[[vk::push_constant]]
cbuffer Constants
{
    float2 kScale;
    float2 kTranslate;
};

[[vk::binding(0)]]
Texture2D MainTexture;
[[vk::binding(0)]]
SamplerState MainSampler;

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
    PSOutput output;
    output.color = MainTexture.Sample(MainSampler, input.uv) * input.color;
    output.luminance = dot(output.color.xyz, 0.333333);
    return output;
}
