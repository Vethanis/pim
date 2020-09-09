
[[vk::binding(0)]]
Texture2D kTexture;

[[vk::binding(0)]]
SamplerState kSampler;

struct VSInput { float2 pos : POSITION; };

struct PSInput
{
    float4 positionCS : SV_Position;
    float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.positionCS = float4(input.pos.xy, 0.0, 1.0);
    output.uv = input.pos.xy * 0.5 + 0.5;
    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    return kTexture.Sample(kSampler, input.uv);
}
