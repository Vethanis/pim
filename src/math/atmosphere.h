#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/sampling.h"

PIM_C_BEGIN

// Rayleigh scattering: blue/orange due to small particles
// Mie scattering: hazy gray due to large particles

typedef struct SkyMedium_s
{
    float rCrust;   // crust radius
    float rAtmos;   // atmosphere radius

    float3 muR;     // rayleigh scattering coefficient (1 / mean free path), at sea level
    float rhoR;     // rayleigh density coefficient (1 / scale height)

    float muM;      // mie scattering coefficient (1 / mean free path), at sea level
    float rhoM;     // mie density coefficient (1 / scale height)
    float gM;       // mean cosine of mie phase function
} SkyMedium;

extern const SkyMedium kEarthAtmosphere;

pim_inline float2 VEC_CALL RaySphereIntersect(float3 ro, float3 rd, float radius)
{
    float b = 2.0f * f3_dot(rd, ro);
    float c = f3_dot(ro, ro) - (radius * radius);
    float d = (b * b) - 4.0f * c;
    if (d <= 0.0f)
    {
        return f2_v(1.0f, -1.0f);
    }
    float nb = -b;
    float sqrtd = sqrtf(d);
    return f2_v((nb - sqrtd) * 0.5f, (nb + sqrtd) * 0.5f);
}

pim_inline float VEC_CALL RayleighPhase(float cosTheta)
{
    return (3.0f / (16.0f * kPi)) * (1.0f + cosTheta * cosTheta);
}

pim_inline float VEC_CALL MiePhase(float cosTheta, float g)
{
    float k = (3.0f / (8.0f * kPi)) * (1.0f - g*g) / (2.0f + g*g);
    float l = 1.0f + g*g - 2.0f * g * cosTheta;
    l = l * sqrtf(l);
    // singularity at g=+-1
    return k * (1.0f + cosTheta*cosTheta) / f1_max(kEpsilon, l);
}

// henyey-greenstein phase function
// g: anisotropy in (-1, 1) => (back-scattering, forward-scattering)
// cosTheta: dot(wo, wi)
pim_inline float VEC_CALL HGPhase(float cosTheta, float g)
{
    float g2 = g * g;
    float nom = 1.0f - g2;
    float denom = 1.0f + g2 + 2.0f * g * cosTheta;
    denom = denom * sqrtf(denom);
    denom = f1_max(denom, kEpsilon);
    return nom / (4.0f * kPi * denom);
}

pim_inline float4 VEC_CALL ImportanceSampleHGPhase(float2 Xi, float g)
{
    float cosTheta;
    if (f1_abs(g) > kMilli)
    {
        float a = -1.0f / (2.0f * g);
        float b = 1.0f + (g * g);
        float nom = 1.0f - g * g;
        float denom = 1.0f + g - 2.0f * g * Xi.x;
        denom = f1_max(denom, kEpsilon);
        float c = nom / denom;
        float u = a * (b - c * c);
        cosTheta = f1_clamp(u, -1.0f, 1.0f);
    }
    else
    {
        cosTheta = Xi.x * 2.0f  - 1.0f;
    }
    float phi = kTau * Xi.y;
    return SphericalToCartesian(cosTheta, phi);
}

pim_inline float3 VEC_CALL Atmosphere(
    const SkyMedium* pim_noalias atmos,
    float3 ro,
    float3 rd,
    float3 lightDir,
    float3 luminance,
    i32 steps)
{
    const float rAtmos = atmos->rAtmos;
    const float rCrust = atmos->rCrust;
    float2 nfAt = RaySphereIntersect(ro, rd, rAtmos);
    float2 nfCr = RaySphereIntersect(ro, rd, rCrust);
    if (nfCr.x < nfCr.y)
    {
        nfAt.y = f1_min(nfAt.y, nfCr.x);
    }
    if (nfAt.x >= nfAt.y)
    {
        return f3_0;
    }

    const i32 viewSteps = steps;
    const i32 lightSteps = steps >> 1;
    // reciprocal of scale height
    const float rhoR = atmos->rhoR;
    const float rhoM = atmos->rhoM;
    // scattering coefficients
    const float3 muR = atmos->muR;
    const float muM = atmos->muM;
    // delta time along view ray
    const float dt_v = (nfAt.y - nfAt.x) / viewSteps;
    const float dstep_l = 1.0f / lightSteps;

    // integral of transmitted optical depth
    float3 tr_r = f3_0;
    float3 tr_m = f3_0;
    // integral of optical depth along view ray
    float odR_v = 0.0f;
    float odM_v = 0.0f;

    for (i32 iView = 0; iView < viewSteps; iView++)
    {
        // mean raytime along view ray segment
        float t_v = (iView + 0.5f) * dt_v;
        // mean position of view ray segment
        float3 pos_v = f3_add(ro, f3_mulvs(rd, t_v));
        // mean height of view segment in atmosphere
        // assumes (0,0,0) is center of planet
        float h_v = f3_length(pos_v) - rCrust;
        if (h_v < 0.0f)
        {
            break;
        }

        // raytime for light ray to exit atmosphere
        float tFar_l = RaySphereIntersect(pos_v, lightDir, rAtmos).y;
        // light ray segment length
        float dt_l = tFar_l * dstep_l;

        // optical depth of light ray
        float odR_l = 0.0f;
        float odM_l = 0.0f;
        for (i32 iLight = 0; iLight < lightSteps; iLight++)
        {
            // mean raytime along light ray segment
            float t_l = (iLight + 0.5f) * dt_l;
            // mean position of light segment
            float3 pos_l = f3_add(pos_v, f3_mulvs(lightDir, t_l));
            // mean height of light segment in atmosphere
            // assumes (0,0,0) is center of planet
            float h_l = f3_length(pos_l) - rCrust;
            if (h_l < 0.0f)
            {
                break;
            }
            // optical depth of light segment
            float odR_li = expf(-h_l * rhoR) * dt_l;
            float odM_li = expf(-h_l * rhoM) * dt_l;
            // integrate light ray optical depth
            odR_l += odR_li;
            odM_l += odM_li;
        }

        // optical depth of view segment
        float odR_vi = expf(-h_v * rhoR) * dt_v;
        float odM_vi = expf(-h_v * rhoM) * dt_v;

        // optical depth thus far along view ray and light ray
        float odR_i = muM * (odM_v + odM_l);
        float3 odM_i = f3_mulvs(muR, odR_v + odR_l);
        // transmittance for current light to eye path
        float3 tr_i = f3_exp(f3_neg(f3_addvs(odM_i, odR_i)));

        // integrate transmitted optical depth
        tr_r = f3_add(tr_r, f3_mulvs(tr_i, odR_vi));
        tr_m = f3_add(tr_m, f3_mulvs(tr_i, odM_vi));

        // integrate optical depth along view ray
        odR_v += odR_vi;
        odM_v += odM_vi;
    }

    // phase function normally goes in inner loop,
    // but optimized to here for a directional light.
    // some parallax is missing due to this simplification.
    float cosTheta = f3_dot(rd, lightDir);
    float phR = RayleighPhase(cosTheta);
    float phM = MiePhase(cosTheta, atmos->gM);

    // scale transmitted light by scattering coeff and phase fns
    tr_r = f3_mul(tr_r, f3_mulvs(atmos->muR, phR));
    tr_m = f3_mulvs(tr_m, atmos->muM * phM);

    // transmitted luminance
    return f3_mul(f3_add(tr_r, tr_m), luminance);
}

pim_inline float3 VEC_CALL EarthAtmosphere(
    float3 ro,
    float3 rd,
    float3 L,
    float3 luminance,
    i32 steps)
{
    // moves outer origin to the north pole
    // inner origin is center of planet
    ro.y += kEarthAtmosphere.rCrust;
    return Atmosphere(
        &kEarthAtmosphere,
        ro,
        rd,
        L,
        luminance,
        steps);
}

PIM_C_END
