#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

typedef struct exposure_s
{
    float aperture;
    float shutterTime;
    float avgLum;
    float offset;
    float deltaTime;
    float adaptRate;
    float minEV100;
    float maxEV100;
    bool manual;
} exposure_t;

pim_inline float VEC_CALL ManualEV100(
    float aperture,
    float shutterTime)
{
    // ISO=100
    return log2f((aperture * aperture) / shutterTime);
}

pim_inline float VEC_CALL AutoEV100(float avgLuminance)
{
    return log2f(avgLuminance * (100.0f / 12.5f));
}

pim_inline float VEC_CALL EV100ToExposure(float ev100)
{
    float maxLum = 1.2f * exp2f(ev100);
    return 1.0f / maxLum;
}

pim_inline float VEC_CALL CalcExposure(exposure_t args)
{
    float ev100 = 0.0f;
    if (args.manual)
    {
        ev100 = ManualEV100(args.aperture, args.shutterTime);
    }
    else
    {
        ev100 = AutoEV100(args.avgLum);
        ev100 = f1_clamp(ev100, args.minEV100, args.maxEV100);
    }
    return EV100ToExposure(ev100 - args.offset);
}

pim_inline float VEC_CALL AdaptLuminance(
    float lum0,
    float lum1,
    float dt,
    float rate)
{
    float t = 1.0f - expf(-dt * rate);
    t = f1_saturate(t);
    return f1_lerp(lum0, lum1, t);
}

void ExposeImage(int2 size, float4* pim_noalias light, exposure_t* parameters);

PIM_C_END
