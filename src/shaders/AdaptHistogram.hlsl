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
    const float minEV = args.minEV;
    const float maxEV = args.maxEV;
    const float minCdf = args.minCdf;
    const float maxCdf = args.maxCdf;
    // i varies from 1 to 255
    // reciprocal range is: 1.0 / 254.0
    const float dEV = (maxEV - minEV) * (1.0 / kBinRange);

    float minLum = FLT_MAX;
    float maxLum = -FLT_MAX;
    float avgLum = 0.0;
    float cdf = 0.0;
    for (uint i = 0; i < kHistogramSize; ++i)
    {
        uint count = HistogramBuffer[i];
        HistogramBuffer[i] = 0;
        float pdf = rcpSamples * count;

        // https://www.desmos.com/calculator/tetw9t35df
        float rcpPdf = rcp(max(kEpsilon, pdf));
        float w0 = 1.0 - saturate((minCdf - cdf) * rcpPdf);
        float w1 = saturate((maxCdf - cdf) * rcpPdf);
        float w = pdf * w0 * w1;

        float ev = BinToEV(i, minEV, dEV);
        float lum = EV100ToLum(ev);
        avgLum += lum * w;
        cdf += pdf;
        maxLum = (count != 0) ? max(maxLum, lum) : maxLum;
        minLum = (count != 0) ? min(minLum, lum) : minLum;
    }

    avgLum = AdaptLuminance(GetAverageLum(), avgLum, args.deltaTime, args.adaptRate);
    float exposure = CalcExposure(args, avgLum);
    SetAverageLum(avgLum);
    SetMaxLum(maxLum);
    SetMinLum(minLum);
    SetExposure(exposure);
}
