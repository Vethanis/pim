#include "Exposure.hlsl"

// storage image at binding 0
[[vk::binding(0)]]
RWTexture2D<half> LumTexture;

// storage buffer at binding 1
[[vk::binding(1)]]
RWStructuredBuffer<uint> HistogramBuffer;

// push constant
[[vk::push_constant]]
cbuffer InputConstants
{
    uint m_inputWidth;
    uint m_inputHeight;
    vkrExposure m_exposure;
};

// Based on https://www.alextardif.com/HistogramLuminance.html

groupshared uint GroupHistogram[kNumBins];

[numthreads(16, 16, 1)]
void CSMain(
    uint localIndex : SV_GroupIndex,
    uint3 globalID : SV_DispatchThreadID)
{
    float minEV = m_exposure.minEV;
    float maxEV = m_exposure.maxEV;
    GroupHistogram[localIndex] = 0;

    GroupMemoryBarrierWithGroupSync();

    if ((globalID.x < m_inputWidth) && (globalID.y < m_inputHeight))
    {
        float lum = LumTexture.Load(globalID.xy).r;
        uint bin = LumToBin(lum, minEV, maxEV);
        InterlockedAdd(GroupHistogram[bin], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(HistogramBuffer[localIndex], GroupHistogram[localIndex]);
}
