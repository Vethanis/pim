
#include "shaders/example_input.hlsl"

// ----------------------------------------------------------------------------
// vertex main

PixelInputs main(VertInputs input)
{
    const float4 proj = kProj;
    const float4 camTS = kCamTS;
    const float4 camQuat = kCamQuat;
    const float4 objTS = kObjTS;
    const float4 objQuat = kObjQuat;
    const float4 uvTS = kObjUvTS;
    PixelInputs output;
    output.Pws = ToWorld(objTS, objQuat, input.Pos);
    output.Nws = q_rot(objQuat, input.Nos);
    float3 Pcs = ToCamera(camTS, camQuat, output.Pws);
    output.Pclip = ToClip(proj, Pcs);
    output.uv = TransformUv(uvTS, input.uv);
    return output;
}
