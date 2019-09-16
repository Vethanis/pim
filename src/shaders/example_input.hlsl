#ifndef EXAMPLE_INPUT_H
#define EXAMPLE_INPUT_H

#include "shaders/transforms.hlsl"

// ----------------------------------------------------------------------------
// constants

cbuffer CameraConstants : register(b0)
{
    float4 kProj;
    float4 kCamTS;
    float4 kCamQuat;
};

cbuffer LightConstants : register(b1)
{
    float4 kLightDir;
    float4 kLightRad;
};

cbuffer ObjectConstants : register(b2)
{
    float4 kObjTS;
    float4 kObjQuat;
    float4 kObjUvTS;
};

// ----------------------------------------------------------------------------
// textures

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-to-type

// prefiltered specular irradiance map
TextureCube specCube : register(t0);
SamplerState specCubeSampler : register(s0);

// preconvolved diffuse irradiance map
TextureCube diffCube : register(t1);
SamplerState diffCubeSampler : register(s1);

// (albedo, alpha) map
Texture2D albedoTexture : register(t2);
SamplerState albedoSampler : register(s2);

// (occlusion, metallic, roughness, 0) map
Texture2D dataTexture : register(t3);
SamplerState dataSampler : register(s3);

// ----------------------------------------------------------------------------
// inter-stage formats

struct VertInputs
{
    float3 Pos : POSITION;
    float3 Nos : NORMAL;
    float2 uv : TEXCOORD0;
};

struct PixelInputs
{
    float4 Pclip : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 Pws : TEXCOORD1;
    float3 Nws : TEXCOORD2;
};

struct FrameInputs
{
    float4 color : SV_TARGET;
};

#endif // EXAMPLE_INPUT_H
