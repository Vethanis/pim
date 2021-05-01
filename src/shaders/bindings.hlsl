#ifndef BINDINGS_HLSL
#define BINDINGS_HLSL

// ----------------------------------------------------------------------------

struct PerCamera
{
    float4x4 worldToClip;
    float4 eye;

    float hdrEnabled;
    float whitepoint;
    float displayNits;
    float uiNits;
};

// uniform buffer at binding 0
[[vk::binding(0)]]
cbuffer cameraData
{
    PerCamera cameraData;
};

float4x4 GetWorldToClip() { return cameraData.worldToClip; }
float3 GetEye() { return cameraData.eye; }
bool HdrEnabled() { return cameraData.hdrEnabled != 0.0; }
float GetWhitepoint() { return cameraData.whitepoint; }
float GetDisplayNits() { return cameraData.displayNits; }
float GetUiNits() { return cameraData.uiNits; }

// ----------------------------------------------------------------------------

// storage image at binding 1
[[vk::binding(1)]]
RWTexture2D<half> LumTexture;

// ----------------------------------------------------------------------------

// storage buffer at binding 2
[[vk::binding(2)]]
RWStructuredBuffer<uint> HistogramBuffer;

// ----------------------------------------------------------------------------

// storage buffer at binding 3
[[vk::binding(3)]]
RWStructuredBuffer<float> ExposureBuffer;

float GetAverageLum() { return ExposureBuffer[0]; }
void SetAverageLum(float x) { ExposureBuffer[0] = x; }
float GetExposure() { return ExposureBuffer[1]; }
void SetExposure(float x) { ExposureBuffer[1] = x; }
float GetMaxLum() { return ExposureBuffer[2]; }
void SetMaxLum(float x) { ExposureBuffer[2] = x; }
float GetMinLum() { return ExposureBuffer[3]; }
void SetMinLum(float x) { ExposureBuffer[3] = x; }

// ----------------------------------------------------------------------------

// combined sampler sampled image table at binding 4
[[vk::binding(4)]]
Texture1D TextureTable1D[];
[[vk::binding(4)]]
SamplerState SamplerTable1D[];

float4 SampleTable1D(uint index, float u)
{
    return TextureTable1D[index].Sample(SamplerTable1D[index], u);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table at binding 5
[[vk::binding(5)]]
Texture2D TextureTable2D[];
[[vk::binding(5)]]
SamplerState SamplerTable2D[];

float4 SampleTable2D(uint index, float2 uv)
{
    return TextureTable2D[index].Sample(SamplerTable2D[index], uv);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table at binding 6
[[vk::binding(6)]]
Texture3D TextureTable3D[];
[[vk::binding(6)]]
SamplerState SamplerTable3D[];

float4 SampleTable3D(uint index, float3 uvw)
{
    return TextureTable3D[index].Sample(SamplerTable3D[index], uvw);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table at binding 7
[[vk::binding(7)]]
TextureCube TextureTableCube[];
[[vk::binding(7)]]
SamplerState SamplerTableCube[];

float4 SampleTableCube(uint index, float3 dir)
{
    return TextureTableCube[index].Sample(SamplerTableCube[index], dir);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table at binding 8
[[vk::binding(8)]]
Texture2DArray TextureTable2DArray[];
[[vk::binding(8)]]
SamplerState SamplerTable2DArray[];

float4 SampleTable2DArray(uint index, float2 uv, int layer)
{
    return TextureTable2DArray[index].Sample(SamplerTable2DArray[index], float3(uv.x, uv.y, layer));
}

// ----------------------------------------------------------------------------

#endif // BINDINGS_HLSL
