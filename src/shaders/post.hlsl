#include "common.hlsl"
#include "Color.hlsl"

struct VsOutput
{
    float4 positionCS : SV_Position;
    float2 uv : TEXCOORD0;
};

VsOutput VSMain(uint id : SV_VertexID)
{
    VsOutput vsOut;
    vsOut.uv = float2((id << 1) & 2, id & 2);
    vsOut.positionCS = float4(vsOut.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return vsOut;
}

float4 PSMain(VsOutput psIn) : SV_Target
{
    psIn.uv.y = 1.0 - psIn.uv.y;
    float3 lum = SceneLuminance.Sample(SceneLuminanceSampler, psIn.uv).rgb;
    lum = ExposeScene(lum);
    return float4(lum, 1.0);
}
