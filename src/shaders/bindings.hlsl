#ifndef BINDINGS_HLSL
#define BINDINGS_HLSL

struct PerCamera
{
    float4x4 worldToClip;
    float4 eye;
    uint lmBegin;
};

// uniform buffer at binding 0
[[vk::binding(0)]]
cbuffer cameraData
{
    PerCamera cameraData;
};

// storage image at binding 1
[[vk::binding(1)]]
RWTexture2D<half> LumTexture;

// storage buffer at binding 2
[[vk::binding(2)]]
RWStructuredBuffer<uint> HistogramBuffer;

// storage buffer at binding 3
[[vk::binding(3)]]
RWStructuredBuffer<float> ExposureBuffer;

// combined sampler sampled image table at binding 4
[[vk::binding(4)]]
Texture1D TextureTable1D[];
[[vk::binding(4)]]
SamplerState SamplerTable1D[];

// combined sampler sampled image table at binding 5
[[vk::binding(5)]]
Texture2D TextureTable2D[];
[[vk::binding(5)]]
SamplerState SamplerTable2D[];

// combined sampler sampled image table at binding 6
[[vk::binding(6)]]
Texture3D TextureTable3D[];
[[vk::binding(6)]]
SamplerState SamplerTable3D[];

// combined sampler sampled image table at binding 7
[[vk::binding(7)]]
TextureCube TextureTableCube[];
[[vk::binding(7)]]
SamplerState SamplerTableCube[];

// combined sampler sampled image table at binding 8
[[vk::binding(8)]]
Texture2DArray TextureTable2DArray[];
[[vk::binding(8)]]
SamplerState SamplerTable2DArray[];

#endif // BINDINGS_HLSL
