#pragma once

#include "common/macro.h"
#include "math/types.h"
#include "math/float2_funcs.h"
#include "math/float3_funcs.h"
#include "math/sampling.h"

PIM_C_BEGIN

// Rayleigh scattering: blue/orange tinting due to air (smaller particles)
// Mies scattering: hazy gray tinting due to aerosols (bigger particles)

typedef struct atmos_s
{
    float crustRadius;
    float atmosphereRadius;
    float3 Br;      // beta rayleigh
    float Bm;       // beta mie
    float Hr;       // rayleigh scale height
    float Hm;       // mie scale height
    float g;        // mean cosine of mie phase function
} atmos_t;

static const atmos_t kEarthAtmosphere =
{
    6360e3f,
    6420e3f,
    { 5.5e-6f, 13.0e-6f, 22.4e-6f },
    21e-6f,
    7994.0f,
    1200.0f,
    0.758f,
};

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
        float c = nom / denom;
        float u = a * (b - c * c);
        cosTheta = f1_clamp(u, -1.0f, 1.0f);
    }
    else
    {
        cosTheta = 2.0f * Xi.x - 1.0f;
    }
    float phi = kTau * Xi.y;
    return SphericalToCartesian(cosTheta, phi);
}

pim_inline float3 VEC_CALL Atmosphere(
    atmos_t atmos,
    float3 ro,
    float3 rd,
    float3 lightDir,
    float3 lightColor,
    i32 steps)
{
    float2 nfAt = RaySphereIntersect(ro, rd, atmos.atmosphereRadius);
    float2 nfCr = RaySphereIntersect(ro, rd, atmos.crustRadius);
    if (nfCr.x < nfCr.y)
    {
        nfAt.y = f1_min(nfAt.y, nfCr.x);
    }
    if (nfAt.x >= nfAt.y)
    {
        return f3_0;
    }
    const i32 iSteps = steps;
    const i32 jSteps = steps >> 1;
    const float rcpHr = 1.0f / atmos.Hr;
    const float rcpHm = 1.0f / atmos.Hm;
    const float iStepSize = (nfAt.y - nfAt.x) / iSteps;
    const float deltaJ = 1.0f / jSteps;

    float3 totalRlh = f3_0;
    float3 totalMie = f3_0;
    float iOdRlh = 0.0f;
    float iOdMie = 0.0f;
    for (i32 i = 0; i < iSteps; i++)
    {
        float3 iPos = f3_add(ro, f3_mulvs(rd, (i + 0.5f) * iStepSize));
        float jStepSize = RaySphereIntersect(iPos, lightDir, atmos.atmosphereRadius).y * deltaJ;

        float jOdRlh = 0.0f;
        float jOdMie = 0.0f;
        for (i32 j = 0; j < jSteps; j++)
        {
            float3 jPos = f3_add(iPos, f3_mulvs(lightDir, (j + 0.5f) * jStepSize));
            float jHeight = f3_length(jPos) - atmos.crustRadius;

            jOdRlh += expf(-jHeight * rcpHr) * jStepSize;
            jOdMie += expf(-jHeight * rcpHm) * jStepSize;
        }

        float mieAttn = atmos.Bm * (iOdMie + jOdMie);
        float3 rlhAttn = f3_mulvs(atmos.Br, iOdRlh + jOdRlh);
        float3 attn = f3_exp(f3_neg(f3_addvs(rlhAttn, mieAttn)));

        float iHeight = f3_length(iPos) - atmos.crustRadius;
        float odStepRlh = expf(-iHeight * rcpHr) * iStepSize;
        float odStepMie = expf(-iHeight * rcpHm) * iStepSize;

        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        totalRlh = f3_add(totalRlh, f3_mulvs(attn, odStepRlh));
        totalMie = f3_add(totalMie, f3_mulvs(attn, odStepMie));
    }

    float cosTheta = f3_dot(rd, lightDir);
    totalRlh = f3_mul(totalRlh, f3_mulvs(atmos.Br, RayleighPhase(cosTheta)));
    totalMie = f3_mulvs(totalMie, atmos.Bm * MiePhase(cosTheta, atmos.g));

    return f3_mul(f3_add(totalRlh, totalMie), lightColor);
}

pim_inline float3 VEC_CALL EarthAtmosphere(
    float3 ro,
    float3 rd,
    float3 L,
    float3 sunIntensity,
    i32 steps)
{
    ro.y += kEarthAtmosphere.crustRadius;
    return Atmosphere(
        kEarthAtmosphere,
        ro,
        rd,
        L,
        sunIntensity,
        steps);
}

PIM_C_END
