#ifndef EXPOSURE_HLSL
#define EXPOSURE_HLSL

#include "common.hlsl"

// ----------------------------------------------------------------------------
// Legend
// ----------------------------------------------------------------------------
// N: relative aperture, in f-stops
// t: Shutter time, in seconds
// S: Sensor sensitivity, in ISO
// q: Proportion of light traveling through lens to sensor

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------
// K: 12.5, standard camera calibration
// q: 0.65, with theta=10deg, T=0.9, v=0.98

// ----------------------------------------------------------------------------
// Equations
// ----------------------------------------------------------------------------
// q = (pi/4) * T * v(theta) * pow(cos(theta), 4)
//
// EV100 = log2(NN/t) when S = 100
//       = log2(NN/t * 100/S)
//       = log2((Lavg * S) / K)

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
    // EV100 = log2((Lavg * S) / K)
    // S = 100
    // K = 12.5
    // S/K = 8
    return log2(Lavg) + 3.0;
}

float EV100ToLum(float ev100)
{
    return exp2(ev100 - 3.0);
}

uint LumToBin(float lum, float minEV, float maxEV)
{
    if (lum > kEpsilon)
    {
        float ev = LumToEV100(lum);
        float t = unlerp(minEV, maxEV, ev);
        // [1, N-1]
        return (uint)(1.5f + t * (R_ExposureHistogramSize - 2));
    }
    return 0;
}

float BinToEV(uint i, float minEV, float maxEV)
{
    if (i != 0)
    {
        const float rcpBinCount = 1.0f / (R_ExposureHistogramSize - 1);
        return lerp(minEV, maxEV, (i - 0.5) * rcpBinCount);
    }
    return kLog2Epsilon;
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

// https://resources.mpi-inf.mpg.de/hdr/peffects/krawczyk05sccg.pdf
float ExposureCompensationCurve(float ev100)
{
    float L = EV100ToLum(ev100);
    float keyValue = 1.03f - 2.0f / (log10(L + 1.0f) + 2.0f);
    const float midGrey = 0.18f;
    return keyValue / midGrey;
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

    float exposureCompensation = ExposureCompensationCurve(ev100);
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
    exposure *= exposureCompensation;

    return exposure;
}

#endif // EXPOSURE_HLSL
