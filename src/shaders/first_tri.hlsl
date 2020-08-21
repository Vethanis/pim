// #extension GL_ARB_separate_shader_objects : enable

struct VSInput
{
    uint vertIndex : SV_VertexID;
};

struct PSInput
{
    float4 positionCS : SV_Position;
    float4 color : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_Target;
};

static const float4 kPositions[] =
{
    float4(0.0, -0.5, 0.0, 1.0),
    float4(0.5, 0.5, 0.0, 1.0),
    float4(-0.5, 0.5, 0.0, 1.0),
};

static const float4 kColors[] =
{
    float4(1.0, 0.0, 0.0, 1.0),
    float4(0.0, 1.0, 0.0, 1.0),
    float4(0.0, 0.0, 1.0, 1.0),
};

PSInput VSMain(VSInput input)
{
    uint id = input.vertIndex % 3;
    PSInput output;
    output.positionCS = kPositions[id];
    output.color = kColors[id];
    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.color = input.color;
    return output;
}
