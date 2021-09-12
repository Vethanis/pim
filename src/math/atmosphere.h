#pragma once

#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/sampling.h"
#include "common/random.h"

PIM_C_BEGIN

// Rayleigh scattering: blue/orange due to small particles
// Mie scattering: hazy gray due to large particles

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
    float sqrtd = sqrtf(d);
    float t0 = (-b - sqrtd) * 0.5f;
    float t1 = (-b + sqrtd) * 0.5f;
    return f2_v(t0, t1);
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
    // assumes (0,0,0) is center of planet
    const float rCrust = atmos->rCrust;

    // reciprocal of scale height
    const float rhoR = atmos->rhoR;
    const float rhoM = atmos->rhoM;
    // scattering coefficients
    const float3 muR = atmos->muR;
    const float muM = atmos->muM;
    const float majorant = f1_max(muM, f3_hmax(muR)) * steps;
    // bias: use median free path instead of randomly sampling it
    const float mfp = SampleFreePath(0.5f, 1.0f / majorant);
    // bias: stop marching at a certain density
    const float kMinDensity = 1e-5f;

    float3 tr_r = f3_0;
    float3 tr_m = f3_0;
    float odR_v = 0.0f;
    float odM_v = 0.0f;
    float t_v = 0.0f;
    while (true)
    {
        float3 pos_v = f3_add(ro, f3_mulvs(rd, t_v));
        float h_v = f3_length(pos_v) - rCrust;
        if (h_v < 0.0f)
        {
            break;
        }
        float densityR_v = expf(-h_v * rhoR);
        float densityM_v = expf(-h_v * rhoM);
        if ((densityR_v + densityM_v) < kMinDensity)
        {
            break;
        }
        float dt_v = mfp;
        t_v += dt_v;

        // in-scattering at view position
        float odR_vi = densityR_v * dt_v;
        float odM_vi = densityM_v * dt_v;
        odR_v += odR_vi;
        odM_v += odM_vi;

        float odR_l = 0.0f;
        float odM_l = 0.0f;
        float t_l = 0.0f;
        bool hitCrust = false;
        while (true)
        {
            float3 pos_l = f3_add(pos_v, f3_mulvs(lightDir, t_l));
            float h_l = f3_length(pos_l) - rCrust;
            if (h_l < 0.0f)
            {
                hitCrust = true;
                break;
            }
            float densityR_l = expf(-h_l * rhoR);
            float densityM_l = expf(-h_l * rhoM);
            if ((densityR_l + densityM_l) < kMinDensity)
            {
                break;
            }
            float dt_l = mfp;
            t_l += dt_l;
            odR_l += densityR_l * dt_l;
            odM_l += densityM_l * dt_l;
        }

        if (!hitCrust)
        {
            // optical depth and transmittance for path from sun to eye
            float3 od_i = f3_addvs(
                f3_mulvs(muR, odR_v + odR_l),
                muM * (odM_v + odM_l));
            float3 tr_i = f3_exp(f3_neg(od_i));

            // transmit in-scattering at view position through
            // transmittance from light to eye at this step.
            tr_r = f3_add(tr_r, f3_mulvs(tr_i, odR_vi));
            tr_m = f3_add(tr_m, f3_mulvs(tr_i, odM_vi));
        }
    }

    {
        // pulled out of loop due to directional light and single-scattering.
        float cosTheta = f3_dot(rd, lightDir);
        float phR = RayleighPhase(cosTheta);
        float phM = MiePhase(cosTheta, atmos->gM);
        // scale transmitted in-scattering by scattering coeff and phase fns
        tr_r = f3_mul(tr_r, f3_mulvs(muR, phR));
        tr_m = f3_mulvs(tr_m, muM * phM);
    }

    // scale transmitted in-scattering by luminance
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
