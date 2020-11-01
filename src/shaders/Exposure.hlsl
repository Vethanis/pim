#ifndef EXPOSURE_HLSL
#define EXPOSURE_HLSL

#include "Macro.hlsl"

#define kNumBins        256

[[vk::binding(3)]]
RWStructuredBuffer<float> ExposureBuffer;

float GetAverageLum() { return ExposureBuffer[0]; }
void SetAverageLum(float lum) { ExposureBuffer[0] = lum; }
float GetExposure() { return ExposureBuffer[1]; }
void SetExposure(float exposure) { ExposureBuffer[1] = exposure; }

struct vkrExposure
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
};

float LumToEV100(float Lavg)
{
    return log2(Lavg * 8.0);
}

float EV100ToLum(float ev100)
{
    return exp2(ev100) / 8.0;
}

uint LumToBin(float lum, float minEV, float maxEV)
{
    if (lum <= kEpsilon)
    {
        return 0;
    }
    float ev = LumToEV100(lum);
    float t = unlerp(minEV, maxEV, ev);
    uint bin = (uint)(1.0 + t * 254.0);
    return bin;
}

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
    lum0 = max(lum0, kEpsilon);
    lum1 = max(lum1, kEpsilon);
    float t = 1.0 - exp(-dt * tau);
    return lerp(lum0, lum1, saturate(t));
}

float ManualEV100(float aperture, float shutterTime, float ISO)
{
    float a = (aperture * aperture) / shutterTime;
    float b = 100.0 / ISO;
    return log2(a * b);
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

float CalcExposure(vkrExposure args, float avgLum)
{
    avgLum = max(avgLum, kEpsilon);
    float ev100;
    if (args.manual != 0)
    {
        ev100 = ManualEV100(args.aperture, args.shutterTime, args.ISO);
    }
    else
    {
        ev100 = LumToEV100(avgLum);
    }

    ev100 = ev100 - args.offsetEV;
    ev100 = clamp(ev100, args.minEV, args.maxEV);

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

#endif // EXPOSURE_HLSL
