#include "Exposure.hlsl"

[[vk::push_constant]]
cbuffer InputConstants
{
    uint m_inputWidth;
    uint m_inputHeight;
    vkrExposure m_exposure;
};

[numthreads(1, 1, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x != 0)
    {
        return;
    }

    const float rcpSamples = 1.0 / (m_inputWidth * m_inputHeight);
    vkrExposure args = m_exposure;
    const float minEV = max(args.minEV, kLog2Epsilon);
    const float maxEV = args.maxEV;
    const float minCdf = args.minCdf;
    const float maxCdf = args.maxCdf;

    float minLum = FLT_MAX;
    float maxLum = -FLT_MAX;
    float avgLum = 0.0;
    float cdf = 0.0;
    for (uint i = 0; i < R_ExposureHistogramSize; ++i)
    {
        uint count = HistogramBuffer[i];
        HistogramBuffer[i] = 0;
        float pdf = rcpSamples * count;

        // https://www.desmos.com/calculator/tetw9t35df
        float rcpPdf = rcp(max(kEpsilon, pdf));
        float w0 = 1.0 - saturate((minCdf - cdf) * rcpPdf);
        float w1 = saturate((maxCdf - cdf) * rcpPdf);
        float w = pdf * w0 * w1;

        float ev = BinToEV(i, minEV, maxEV);
        float lum = EV100ToLum(ev);
        avgLum += lum * w;
        cdf += pdf;
        maxLum = max(maxLum, lum);
        minLum = min(minLum, lum);
    }

    avgLum = AdaptLuminance(GetAverageLum(), avgLum, args.deltaTime, args.adaptRate);
    float exposure = CalcExposure(args, avgLum);
    SetAverageLum(avgLum);
    SetMaxLum(maxLum);
    SetMinLum(minLum);
    SetExposure(exposure);
}
