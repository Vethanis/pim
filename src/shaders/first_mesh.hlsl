struct VSInput
{
    float4 positionOS : POSITION;
    float4 normalOS : NORMAL;
    float4 uv01 : TEXCOORD0;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float4 positionWS : TEXCOORD0;
    float4 normalWS : TEXCOORD1;
    float4 uv01 : TEXCOORD2;
};

struct PSOutput
{
    float4 color : SV_Target;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.positionCS = input.positionOS;
    output.positionWS = input.positionOS;
    output.normalWS = input.normalOS;
    output.uv01 = input.uv01;
    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.color.xy = input.uv01.xy;
    output.color.z = 0.0;
    output.color.w = 1.0;
    return output;
}
