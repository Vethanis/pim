#include "Exposure.hlsl"

// push constant
[[vk::push_constant]]
cbuffer InputConstants
{
    uint m_inputWidth;
    uint m_inputHeight;
    vkrExposure m_exposure;
};

// Based on https://www.alextardif.com/HistogramLuminance.html

groupshared uint gs_histogram[kHistogramSize];

// (numthreadsX * numthreadsY) must be == kHistogramSize
[numthreads(16, 16, 1)]
void CSMain(
    uint localId : SV_GroupIndex, // [0, (numthreadsX * numthreadsY * numthreadsZ) - 1]
    uint3 tid : SV_DispatchThreadID)
{
    const float minEV = m_exposure.minEV;
    const float maxEV = m_exposure.maxEV;
    const uint2 inputSize = uint2(m_inputWidth, m_inputHeight);

    gs_histogram[localId] = 0;

    GroupMemoryBarrierWithGroupSync();

    if (all(tid.xy < inputSize))
    {
        float lum = LumTexture.Load(tid.xy).r;
        uint bin = LumToBin(lum, minEV, maxEV);
        InterlockedAdd(gs_histogram[bin], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(HistogramBuffer[localId], gs_histogram[localId]);
}
