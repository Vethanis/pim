#ifndef BINDINGS_HLSL
#define BINDINGS_HLSL

// ----------------------------------------------------------------------------

// uniform buffer
[[vk::binding(bid_Globals)]]
cbuffer GlobalsBuffer
{
    float4x4 g_WorldToClip;
    float4 g_Eye;

    float g_HdrEnabled;
    float g_Whitepoint;
    float g_DisplayNits;
    float g_UiNits;

    uint2 g_RenderSize;
    uint2 g_DisplaySize;
};

float4x4 GetWorldToClip() { return g_WorldToClip; }
float3 GetEye() { return g_Eye.xyz; }
bool HdrEnabled() { return g_HdrEnabled != 0.0; }
float GetWhitepoint() { return g_Whitepoint; }
float GetDisplayNits() { return g_DisplayNits; }
float GetUiNits() { return g_UiNits; }
uint2 GetRenderSize() { return g_RenderSize; }
uint2 GetDisplaySize() { return g_DisplaySize; }

// ----------------------------------------------------------------------------

// combined sampler sampled image table
[[vk::binding(bid_SceneLuminance)]]
Texture2D<float4> SceneLuminance;
[[vk::binding(bid_SceneLuminance)]]
SamplerState SceneLuminanceSampler;

// ----------------------------------------------------------------------------

// storage image
[[vk::binding(bid_RWSceneLuminance)]]
RWTexture2D<float4> RWSceneLuminance;

// ----------------------------------------------------------------------------

// storage buffer
[[vk::binding(bid_HistogramBuffer)]]
RWStructuredBuffer<uint> HistogramBuffer;

// ----------------------------------------------------------------------------

// storage buffer
[[vk::binding(bid_ExposureBuffer)]]
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

// combined sampler sampled image table
[[vk::binding(bid_TextureTable1D)]]
Texture1D TextureTable1D[];
[[vk::binding(bid_TextureTable1D)]]
SamplerState SamplerTable1D[];

float4 SampleTable1D(uint index, float u)
{
    return TextureTable1D[index].Sample(SamplerTable1D[index], u);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table
[[vk::binding(bid_TextureTable2D)]]
Texture2D TextureTable2D[];
[[vk::binding(bid_TextureTable2D)]]
SamplerState SamplerTable2D[];

float4 SampleTable2D(uint index, float2 uv)
{
    return TextureTable2D[index].Sample(SamplerTable2D[index], uv);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table
[[vk::binding(bid_TextureTable3D)]]
Texture3D TextureTable3D[];
[[vk::binding(bid_TextureTable3D)]]
SamplerState SamplerTable3D[];

float4 SampleTable3D(uint index, float3 uvw)
{
    return TextureTable3D[index].Sample(SamplerTable3D[index], uvw);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table
[[vk::binding(bid_TextureTableCube)]]
TextureCube TextureTableCube[];
[[vk::binding(bid_TextureTableCube)]]
SamplerState SamplerTableCube[];

float4 SampleTableCube(uint index, float3 dir)
{
    return TextureTableCube[index].Sample(SamplerTableCube[index], dir);
}

// ----------------------------------------------------------------------------

// combined sampler sampled image table
[[vk::binding(bid_TextureTable2DArray)]]
Texture2DArray TextureTable2DArray[];
[[vk::binding(bid_TextureTable2DArray)]]
SamplerState SamplerTable2DArray[];

float4 SampleTable2DArray(uint index, float2 uv, int layer)
{
    return TextureTable2DArray[index].Sample(SamplerTable2DArray[index], float3(uv.x, uv.y, layer));
}

// ----------------------------------------------------------------------------

#endif // BINDINGS_HLSL
