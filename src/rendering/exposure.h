#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"
#include "rendering/vulkan/vkr.h"

PIM_C_BEGIN

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

pim_inline float ManualEV100(
    float aperture,
    float shutterTime,
    float ISO)
{
    // EVs = log2(NN/t)
    // EV100 = EVs - log2(S/100)
    // log(a) - log(b) = log(a * 1/b)
    float a = (aperture * aperture) / shutterTime;
    float b = 100.0f / ISO;
    return log2f(a * b);
}

pim_inline float LumToEV100(float Lavg)
{
    // EV100 = log2((Lavg * S) / K)
    // S = 100
    // K = 12.5
    // S/K = 8
    return log2f(Lavg * 8.0f);
}

// https://www.desmos.com/calculator/vorr8hwdl7
pim_inline float EV100ToLum(float ev100)
{
    return exp2f(ev100) / 8.0f;
}

pim_inline float SaturationExposure(float ev100)
{
    // The factor 78 is chosen such that exposure settings based on a standard
    // light meter and an 18-percent reflective surface will result in an image
    // with a saturation of 12.7%
    // EV100 = log2(NN/t)
    // Lmax = (78/(Sq)) * (NN/t)
    // Lmax = (78/(100 * 0.65)) * 2^EV100
    // Lmax = 1.2 * 2^EV100
    const float factor = 78.0f / (100.0f * 0.65f);
    float Lmax = factor * exp2f(ev100);
    return 1.0f / Lmax;
}

pim_inline float StandardExposure(float ev100)
{
    const float midGrey = 0.18f;
    const float factor = 10.0f / (100.0f * 0.65f);
    float Lavg = factor * exp2f(ev100);
    return midGrey / Lavg;
}

pim_inline float CalcExposure(const vkrExposure* args)
{
    float ev100 = 0.0f;
    if (args->manual)
    {
        ev100 = ManualEV100(args->aperture, args->shutterTime, args->ISO);
    }
    else
    {
        ev100 = LumToEV100(args->avgLum);
    }

    ev100 = ev100 - args->offsetEV;

    float exposure;
    if (args->standard)
    {
        exposure = StandardExposure(ev100);
    }
    else
    {
        exposure = SaturationExposure(ev100);
    }
    return exposure;
}

pim_inline float AdaptLuminance(
    float lum0,
    float lum1,
    float dt,
    float rate)
{
	lum0 = f1_max(lum0, kEpsilon);
	lum1 = f1_max(lum1, kEpsilon);
    float t = 1.0f - expf(-dt * rate);
    return f1_lerp(lum0, lum1, f1_sat(t));
}

void ExposeImage(int2 size, float4* pim_noalias light, vkrExposure* parameters);

PIM_C_END
