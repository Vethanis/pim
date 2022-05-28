
[[vk::push_constant]]
cbuffer push_constants
{
    float4x4 g_LocalToClip;
};

struct VSInput
{
    float4 positionWS : POSITION;
};

struct PSInput
{
    float4 positionCS : SV_Position;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.positionCS = mul(g_LocalToClip, float4(input.positionWS.xyz, 1.0));
    return output;
}

void PSMain(PSInput input)
{

}

// "fragment shader writes to output location 0 with no matching attachment"
//float4 PSMain(PSInput input) : SV_Target
//{
//    return 0.0;
//}
