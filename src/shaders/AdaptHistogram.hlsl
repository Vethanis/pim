
#define kNumBins        256
#define kEpsilon        2.38418579e-7
#define i32             int

typedef struct vkrExposure
{
    float exposure;
    float avgLum;
    float deltaTime;
    float adaptRate;

    float aperture;
    float shutterTime;
    float ISO;

    // offsets the output exposure, in EV
    float offsetEV;
    // EV range to consider
    float minEV;
    float maxEV;
    // range of the cdf to consider
    float minCdf;
    float maxCdf;

    // manual: ev100 is based on camera parameters
    // automatic: ev100 is based on luminance
    i32 manual;
    // standard output or saturation based exposure
    i32 standard;
} vkrExposure;

[[vk::binding(1)]]
RWStructuredBuffer<uint> InputHistogram;

[[vk::binding(2)]]
RWStructuredBuffer<vkrExposure> Args;

[[vk::push_constant]]
cbuffer InputConstants
{
    uint m_inputWidth;
    uint m_inputHeight;
};

float BinToEV(uint i, float minEV, float dEV)
{
    // i varies from 1 to 255
    // reciprocal range is: 1.0 / 254.0
    float ev = minEV + (i - 1) * dEV;
    // log2(kEpsilon) == -22
    ev = i > 0 ? ev : -22.0;
    return ev;
}

float AdaptLuminance(float lum0, float lum1, float dt, float tau)
{
    float t = 1.0 - exp(-dt * tau);
    return lerp(lum0, lum1, saturate(t));
}

float ManualEV100(float aperture, float shutterTime, float ISO)
{
    float a = (aperture * aperture) / shutterTime;
    float b = 100.0 / ISO;
    return log2(a * b);
}

float LumToEV100(float Lavg)
{
    return log2(Lavg * 8.0);
}

float EV100ToLum(float ev100)
{
    return exp2(ev100) / 8.0;
}

float SaturationExposure(float ev100)
{
    const float factor = 78.0 / (100.0 * 0.65);
    float Lmax = factor * exp2(ev100);
    return 1.0 / Lmax;
}

float StandardExposure(float ev100)
{
    const float midGrey = 0.18;
    const float factor = 10.0 / (100.0 * 0.65);
    float Lavg = factor * exp2(ev100);
    return midGrey / Lavg;
}

float CalcExposure(vkrExposure args)
{
    float ev100;
    if (args.manual != 0)
    {
        ev100 = ManualEV100(args.aperture, args.shutterTime, args.ISO);
    }
    else
    {
        ev100 = LumToEV100(args.avgLum);
    }

    ev100 = ev100 - args.offsetEV;

    float exposure;
    if (args.standard != 0)
    {
        exposure = StandardExposure(ev100);
    }
    else
    {
        exposure = SaturationExposure(ev100);
    }
    return exposure;
}

[numthreads(1, 1, 1)]
void CSMain()
{
    const float rcpSamples = 1.0 / (m_inputWidth * m_inputHeight);
    vkrExposure args = Args[0];
    const float minEV = args.minEV;
    const float maxEV = args.maxEV;
    const float minCdf = args.minCdf;
    const float maxCdf = args.maxCdf;
    // i varies from 1 to 255
    // reciprocal range is: 1.0 / 254.0
    const float dEV = (maxEV - minEV) * 0.00393700787;

    float avgLum = 0.0;
    float cdf = 0.0;
    for (uint i = 0; i < kNumBins; ++i)
    {
        uint count = InputHistogram[i];
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
    }

    args.avgLum = AdaptLuminance(args.avgLum, avgLum, args.deltaTime, args.adaptRate);
    args.exposure = CalcExposure(args);
    Args[0] = args;
}
