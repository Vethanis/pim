#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/scalar.h"

PIM_C_BEGIN

typedef struct exposure_s
{
    // manual: ev100 is based on camera parameters
    // automatic: ev100 is based on luminance
    bool manual;
    // standard output or saturation based exposure
    bool standard;

    float aperture;
    float shutterTime;
    float ISO;

    float avgLum;
    float deltaTime;
    float adaptRate;

    // offsets the output exposure, in EV
    float offsetEV;
    // range of the cdf to consider
    float histMinProb;
    float histMaxProb;
} exposure_t;

void ExposeImage(int2 size, float4* pim_noalias light, exposure_t* parameters);

PIM_C_END
