
struct VSInput
{
    float4 positionOS : POSITION;
};

struct PSInput
{
    float4 positionCS : SV_Position;
};

[[vk::push_constant]]
cbuffer push_constants
{
    float4x4 objectToClip;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.positionCS = mul(objectToClip, float4(input.positionOS.xyz, 1.0));
    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    return 0.0;
}
