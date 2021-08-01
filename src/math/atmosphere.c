#include "math/atmosphere.h"

const SkyMedium kEarthAtmosphere =
{
    .rCrust = 6360e3f, // 6360km
    .rAtmos = 6420e3f, // 6420km

    .muR =
    {
        1.0f / 192428.0f,   // 192km mfp red, rayleigh
        1.0f / 82354.0f,    // 82km mfp green, rayleigh
        1.0f / 33732.0f,    // 34km mfp blue, rayleigh
    },
    .rhoR = 1.0f / 8500.0f, // 8.5km mean radius, rayleigh

    .muM = 1.0f / 47619.0f, // 48km mfp, mie
    .rhoM = 1.0f / 1200.0f, // 1.2km mean radius, mie
    .gM = 0.758f,
};
