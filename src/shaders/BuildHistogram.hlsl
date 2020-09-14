// Based on https://www.alextardif.com/HistogramLuminance.html

#define kNumBins        256
#define kThreadsX       16
#define kThreadsY       16
#define kThreadsZ       1
#define kEpsilon        2.38418579e-7

// storage image at binding 0
[[vk::binding(0)]]
RWTexture2D<half> InputTexture;

// storage buffer at binding 1
[[vk::binding(1)]]
RWStructuredBuffer<uint> OutputHistogram;

// push constant
[[vk::push_constant]]
cbuffer InputConstants
{
    uint m_inputWidth;
    uint m_inputHeight;
    float m_minEV;
    float m_maxEV;
};

groupshared uint GroupHistogram[kNumBins];

float unlerp(float a, float b, float x)
{
    return saturate((x - a) / (b - a));
}

float LumToEV100(float Lavg)
{
    // EV100 = log2((Lavg * S) / K)
    // S = 100
    // K = 12.5
    // S/K = 8
    return log2(Lavg * 8.0f);
}

uint CalculateBin(half lum, float minEV, float maxEV)
{
    if (lum < kEpsilon)
    {
        return 0;
    }
    float ev = LumToEV100(lum);
    float t = unlerp(minEV, maxEV, ev);
    uint bin = (uint)(1.0 + t * 254.0);
    return bin;
}

[numthreads(kThreadsX, kThreadsY, kThreadsZ)]
void CSMain(
    uint localIndex : SV_GroupIndex,
    uint3 globalID : SV_DispatchThreadID)
{
    GroupHistogram[localIndex] = 0;

    GroupMemoryBarrierWithGroupSync();

    if ((globalID.x < m_inputWidth) && (globalID.y < m_inputHeight))
    {
        half lum = InputTexture.Load(globalID.xy).r;
        uint bin = CalculateBin(lum, m_minEV, m_maxEV);
        InterlockedAdd(GroupHistogram[bin], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(OutputHistogram[localIndex], GroupHistogram[localIndex]);
}
